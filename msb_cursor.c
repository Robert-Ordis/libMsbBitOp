#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>



#include "./include/msb_bitop/msb_cursor.h"
//static群

//nthシークの本体
static inline ssize_t seek_to_nth_bit_(msb_cursor_t *s, size_t nth){
	size_t limit = msb_cursor_get_total_bits(s);
	if(nth >= limit){
		//限界に達しているため、EOBのようなものになる
		nth = limit;
	}
	size_t ret = nth - msb_cursor_get_bit_index(s);
	s->t_oct_index = nth / 8;
	s->t_oct_bit = nth & 0x07;
	return ret;
}

static inline uint32_t get_int32_(msb_cursor_t *s, size_t onset_bit, size_t width_bit){
	if(width_bit < 1){
		return 0;
	}
	
	uint32_t	ret = 0;
	
	//実際に読み出すバイト数を測る。（例えば5bitなら/8すると０だし、でも実際は5ビットを読み出す）
	int				width_mod = width_bit & 0x07;
	int				width_byte = (int)width_bit / 8 + (width_mod > 0);
	
	//配列のインデックスを見る
	int				onset_byte = onset_bit / 8;
	
	//オクテット境界の左と右とを合わせて読んでいくためのインデックス
	int				i;
	int				index;
	
	//onset_bit % 8を取ることで、開始地点が何ビット目か（＝オクテット境界の基準）を図る
	size_t	split_bit = onset_bit & 0x07;	//左側の読み出す幅。右側のシフト幅
	
	
	//printf("%s, onset:%d, width_bit:%d, width_byte:%d\n", __func__, onset_bit, width_bit, width_byte);
	//printf("%s, split_bit:%d, width_mod:%d\n", __func__, split_bit, width_mod);
	//右側のマスキングと左側のマスキングを生成する
	//マスキングの位置基準は読み出し点を0ビットとした物
	/*		オクテット境界をまたいだビット配列の値読み出しについて（必要なんです）
	 *		01234567 | 01234567とし、インデックス3から値を読み出すのなら以下の通り。
	 *		XXX34567 | 012XXXXX
	 *	→	34567|012
	 *		右側は左から(8-インデックス%8)の分だけマスキング。その後インデックス%8の分をシフトする
	 *		左側は右からインデックス%8の分マスキング（右と逆）。その後(8-インデックス%8)分シフト
	 */
	uint8_t			r_mask = 0xFF << (8 - (split_bit));
	uint8_t			l_mask = ~r_mask;
	
	//printf("%s: l_mask->0x%02X, r_mask->0x%02X\n", __func__, l_mask, r_mask);
	
	for(i=0; i<width_byte; i++){
		ret = ret << 8;
		index = onset_byte + i;
		//左側の値（バッファ上限を超えない場合に読み出す）
		if(index < s->m_buf_len){
			ret += ((s->m_buf[index] & l_mask) << split_bit);
		}
		index ++;
		//右側の値（右側のマスキングが行われ、かつバッファ上限を超えていない場合に読み出す）
		if(r_mask && index < s->m_buf_len){
			ret += ((s->m_buf[index] & r_mask) >> (8 - split_bit));
		}
	}
	
	//最後にこのままの値では「ビット幅を指定した整数値」にはならないので、右側のゼロパディングを切り落とす。
	//printf("%s ret(now)->0x%08X\n", __func__, ret);
	return ret >> (width_mod ? (8 - width_mod) : 0);
}


//ビット読み込みの本体
static inline int get_bit_(msb_cursor_t *s, size_t onset_bit){
	//バッファ配列のインデックス
	int	onset_byte = onset_bit / 8;
	//インデックス内のビット位置（MSB=0）
	int	onoct_bit = onset_bit & 0x07;
	int ret = 0;
	if(onset_byte < s->m_buf_len){
		ret = s->m_buf[onset_byte] >> (7 - onoct_bit);
	}
	return ret & 0x01;
}


//32ビットの書き込み本体処理
static inline void overwrite_int32_(msb_cursor_t *s, size_t onset_bit, size_t width_bit, uint32_t value){
	if(width_bit > 32 || width_bit < 1){
		return;
	}
	//printf("%s, onset:%d, width:%d, value:0x%02X(%d)\n", __func__, onset_bit, width_bit, value, value);
	//ないとは思われるが、ビット幅を超えた値については左側を切り捨てる。
	//value &= ~(0xFFFFFFFF << width_bit);
	//width_bitをはみ出したら左側を切り捨てて値を書き込む。
	//処理を簡単にするため、左を切り詰めつつビットを上方向に寄せる
	uint32_t	tmp = value << (32 - width_bit);
	
	//書き込み始めの位置を特定させておく
	int onset_byte = onset_bit / 8;
	int split_bit = onset_bit & 0x07;
	int index = onset_byte;
	
	//値読み込みと同じようにマスキングを施す（今度のマスキングは書き込む場所についてのもの）
	//マスキングの右左の基準はvalueの1オクテットによる。
	uint8_t			r_mask = 0xFF << (8 - (split_bit));
	uint8_t			l_mask = ~r_mask;
	
	//printf("%s: l_mask->0x%02X, r_mask->0x%02X\n", __func__, l_mask, r_mask);
	
	for(tmp = tmp; tmp > 0; tmp = tmp << 8){
		uint8_t	tmp8 = (tmp >> 24) & 0x000000FF;	//最上位8ビットについての計算を行い続ける
		//printf("%s: current byte:0x%02X(%d)\n", __func__, tmp8, tmp8);
		//左側のオクテットを書き換える。
		if(index >= s->m_buf_len){
			//ただし、バッファからはみ出るのなら終了する
			break;
		}
		s->m_buf[index] &= (~l_mask);	//左側のオクテットの書き込み範囲をゼロにする
		s->m_buf[index] |= ((tmp8 >> split_bit) & l_mask);	//書き込み範囲にの分だけ書き込む
		index++;								//インデックスを一つ進める
		
		//右側オクテットを書き換える
		if(index >= s->m_buf_len){
			//ただし、バッファからはみ出るのなら終了する
			break;
		}
		s->m_buf[index] &= (~r_mask);
		s->m_buf[index] |= ((tmp8 << (8 - split_bit)) & r_mask);
	}
	
}


///初期化系列
/**
 *  \brief ビット配列の初期化
 *  
 *  \param		初期化したいビット配列。NULLの場合はmallocで生成
 *  \return	初期化したビット配列構造体
 *  
 *  \details この段階ではまだバッファを指定しません。
 */
msb_cursor_t	*msb_cursor_init(msb_cursor_t *s){
	
	s->m_buf = NULL;
	s->m_buf_len = 0;
	
	s->t_oct_bit = 0;
	s->t_oct_index = 0;
	return s;
}

/**
 *  \brief 文字列バッファを設定する
 *  
 *  \param [in] s 扱うビット配列
 *  \param [in] buf 設定したい文字列。NULLを指定するとmalloc生成
 *  \param [in] len 文字列バッファの長さ[bytes]。未指定に戻す場合は０を指定。
 *  \return 今回扱う文字列バッファのビット単位の長さ
 *  
 *  \details シーク作業用の一時変数群はリセットされます
 */
size_t			msb_cursor_load(msb_cursor_t *s, uint8_t *buf, size_t len){
	
	s->m_buf_len = len;
	s->m_buf = buf;
	s->t_oct_bit = 0;
	s->t_oct_index = 0;
	return len * 8;
}


///長さ取得系列
/**
 *  \brief ビット配列の長さをバイト単位で取得する
 *  
 *  \param		対象のビット配列
 *  \return	現在扱っている配列長[bytes]
 *  
 */
size_t	msb_cursor_get_total_bytes(msb_cursor_t *s){
	return s->m_buf_len;
}

/**
 *  \brief ビット配列の長さをビット単位で取得する
 *  
 *  \param		対象のビット配列
 *  \return	現在扱っている配列長[bits]
 *  
 */
size_t	msb_cursor_get_total_bits(msb_cursor_t *s){
	return s->m_buf_len * 8;
} 


///位置を直接指定したデータ操作API
/**
 *  \brief 読み込み開始点を直接指定してint値を読み込む（符号なし）
 *  
 *  \param		s			対象のビット配列
 *  \param		onset_bit	読み込み開始地点のビット
 *  \param		width_bit	開始地点から読み込む幅（0を指定すると0が帰ります）
 *  \return	現在扱っている配列長[bits]
 *  
 */
uint32_t		msb_cursor_read_nth_uint32(msb_cursor_t *s, size_t onset_bit, size_t width_bit){
	return get_int32_(s, onset_bit, width_bit);
}

/**
 *  \brief 読み込み開始点を直接指定してビットを取得する
 *  
 *  \param		s			対象のビット配列
 *  \param		onset_bit	読み込み開始地点のビット
 *  \param		width_bit	開始地点から読み込む幅（0を指定すると0が帰ります）
 *  \return	指定したビットの値
 *  
 *  \details	枠からはみ出した場合は-1を返します
 *  
 */
int				msb_cursor_read_nth_bit(msb_cursor_t *s, size_t onset_bit){
	return get_bit_(s, onset_bit);
}


/**
 *  \brief 読み込み開始点を直接指定し、そこから指定した幅の情報を文字列バッファに読み出す
 *  
 *  \param [in] s			対象のビット配列
 *  \param [in] onset_bit	開始点のビット
 *  \param [in] width_bit	開始点の読み込むビット幅
 *  \param [in] buf		書き込み先のバッファ
 *  \param [in] buf_len	書き込み先のバッファ長[bytes]
 *  \return	実際に読み込んだビット数
 *  
 *  \remarks	オーバーフローを起こす場合には右側(=LSB側)を捨てます。
 */
size_t			msb_cursor_read_nth_to_buf(msb_cursor_t *s, size_t onset_bit, size_t width_bit, uint8_t *buf, size_t buf_len){
	//仮実装
	
	uint32_t		tmp_read_value;
	size_t	tmp_seek_bit = onset_bit;
	size_t	total_bits = msb_cursor_get_total_bits(s);
	int i = 0;
	//printf("%s: buf_len: %d\n", __func__, buf_len);
	while(width_bit > 0 && i < buf_len && tmp_seek_bit < total_bits){
		//8ビットずつ読み出す
		tmp_read_value = get_int32_(s, tmp_seek_bit, 8);
		buf[i] = (uint8_t)(tmp_read_value);
		//printf("%s: width:%d, value: 0x%02X, out_index: %d\n", __func__, width_bit, tmp_read_value, i);
		//もし、ここの時点で残りビット幅が8を切ってたら、補正を入れる。
		if(width_bit < 8){
			//buf[i] = buf[i] << (8 - width_bit);
			width_bit = 0;
			i++;
			continue;
		}
		width_bit -= 8;
		tmp_seek_bit += 8;
		i++;
	}
	return i;
}


/**
 *  \brief 符号なし整数値を、開始点とビット幅を指定して上書きする
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] onset_bit 書き込み初めのビット
 *  \param [in] width_bit 書き込む幅
 *  \param [in] value 書き込みたい値。
 *  
 *  \details 書き込む値に対してビット幅が足りない場合は左側（上側）が切り詰められます
 */
void			msb_cursor_overwrite_nth_uint32(msb_cursor_t *s, size_t onset_bit, size_t width_bit, uint32_t value){
 overwrite_int32_(s, onset_bit, width_bit, value);
}


///シークしながら行う処理系統のためのAPI
/**
 *  \brief 現在シーク中のバイトを取得する
 *  
 *  \param [in] s 対象のビット配列
 *  \return 現在シーク中のバイトインデックス
 *  
 *  \details 例えば10ビット目＝1バイト+2ビット目の場合は0バイトを返す。
 */
size_t	msb_cursor_get_byte_index(msb_cursor_t *s){
	return s->t_oct_index;
}
/**
 *  \brief ビット単位での、現在のシークポイントを取得する
 *  
 *  \param [in] s 対象のビット配列
 *  \return ビット単位の現在のシークポイント（ゼロスタート）
 *  
 */
size_t	msb_cursor_get_bit_index(msb_cursor_t *s){
	return s->t_oct_index * 8 + s->t_oct_bit;
}

/**
 *  \brief ビット単位での残りシーク可能数を取得する
 *  
 *  \param [in] s 対象のビット配列
 *  \return あと何ビットシークできるか[bits]
 *  
 */
size_t	msb_cursor_get_remain_bits(msb_cursor_t *s){
	if(s->m_buf_len <= 0){
		return 0;
	}
	size_t seeked_bit = s->t_oct_index * 8 + s->t_oct_bit;
	return s->m_buf_len * 8 - seeked_bit;
}

/**
 *  \brief １ビットシークさせる
 *  
 *  \param [in] s 対象のビット配列
 *  \return 実際にシークできたビット数（０でシークできなかった）
 *  
 */
ssize_t	msb_cursor_seek_to_next_bit(msb_cursor_t *s){
	if(msb_cursor_get_remain_bits(s) > 0){
		s->t_oct_bit ++;
		s->t_oct_bit &= 0x07;
		if(s->t_oct_bit == 0){
			s->t_oct_index ++;
		}
		return 1;
	}
	return 0;
}


/**
 *  \brief 指定したビットにシークさせる
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] nth シーク先のビット位置
 *  \return 実際にシークさせた場所
 *  
 *  \details 範囲外にシークさせた場合にはそれぞれの端にシークされます。逆に言えばnth=returnで正常シーク。
 */

ssize_t		msb_cursor_seek_to_nth_bit(msb_cursor_t *s, size_t nth){
	return seek_to_nth_bit_(s, nth);
}

/**
 *  \brief 現在地からNビットシークさせる
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] n シークさせるビット数
 *  \return 実際にシークできたビット数（０でシークできなかった）
 *  
 */
ssize_t		msb_cursor_seek_nbits(msb_cursor_t *s, ssize_t n){
	ssize_t curr = msb_cursor_get_bit_index(s);
	if((curr + n) < 0){
		n = 0 - curr;
	}
	size_t dest = (size_t)(curr + n);
	return seek_to_nth_bit_(s, dest);;
}


/**
 *  \brief 現在のシーク位置から次のバイトへ進める
 *  
 *  \param [in] s 対象のビット配列
 *  \return 実際にシークしたビット数
 *  
 *  \details 0ビット目で呼んだ場合は8ビット進みます
 */
ssize_t		msb_cursor_seek_to_next_byte(msb_cursor_t *s){
	size_t n = 8 - s->t_oct_bit;
	return msb_cursor_seek_nbits(s, n);
}

/**
 *  \brief 現在のシーク位置から次のバイト境界へ進める
 *  
 *  \param [in] s 対象のビット配列
 *  \return 実際にシークしたビット数
 *  
 *  \details 0ビット目でこれを呼んだ場合は0ビットシークされます。
 */
ssize_t		msb_cursor_seek_to_byte_align(msb_cursor_t *s){
	size_t n = 8 - s->t_oct_bit;
	//printf("%s: seek:%d\n", __func__, n);
	if(n == 8){
		//自身が今バイト境界にいる場合は何もしない
		return 0;
	}
	return msb_cursor_seek_nbits(s, n);
}

/**
 *  \brief 現在のシーク位置からNビット指定した整数値を読み出す
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] width_bit 読み出したいビット幅
 *  \return 読み出した値
 *  
 *  \details はみ出した場合は右にゼロパディングした値を読み出します。
 */
uint32_t			msb_cursor_read_uint32(msb_cursor_t *s, size_t width_bit){
	size_t curr_bit = msb_cursor_get_bit_index(s);
	return get_int32_(s, curr_bit, width_bit);
}

/**
 *  \brief 符号なし整数値を、現在のシーク位置からビット幅を指定して上書きする
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] width_bit 書き込む幅
 *  \param [in] value 書き込みたい値。マイナス値指定の場合は対象ビットをすべて1にします。
 *  
 *  \details 書き込む値に対してビット幅が足りない場合は左側（上側）が切り詰められます
 */
void			msb_cursor_overwrite_uint32(msb_cursor_t *s, size_t width_bit, uint32_t value){
	size_t curr_bit = msb_cursor_get_bit_index(s);
 overwrite_int32_(s, curr_bit, width_bit, value);
}

/**
 *  \brief 現在のシーク位置から最後までのデータを指定バッファに書き込む
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] width_bit ビット幅
 *  \param [in] buf 書き込み先バッファ
 *  \param [in] buflen バッファの長さ[bytes]
 *  \return 読み出したバイト数
 *  
 *  \details width_bit < buflen * 8の場合は残りをゼロパディングします
 */
size_t			msb_cursor_read_to_buf(msb_cursor_t *s, size_t width_bit, uint8_t *buf, size_t buflen){
	size_t curr_bit = msb_cursor_get_bit_index(s);
	return msb_cursor_read_nth_to_buf(s, curr_bit, width_bit, buf, buflen);
}


/**
 *  \brief デバッグ用print関数
 *  
 *  \param [in] s 対象のビット配列
 *  
 *  \details デバッグ用
 */
#include <stdio.h>
void			msb_cursor_dbg_print(msb_cursor_t *s){
	size_t i;
	size_t b;
	uint8_t		mask;
	char bit_str[9];
	char option[64];
	uint8_t *buf = s->m_buf;
	
	MSB_DBG_PRINTF("###debug output of msb_cursor_t[%p]\n", s);
	MSB_DBG_PRINTF("buf_len->%zd\n\n", 
		s->m_buf_len
	);
	MSB_DBG_PRINTF("index:%zd, inoct_bit:%zd->seek:%zd bits\n", s->t_oct_index, s->t_oct_bit, msb_cursor_get_bit_index(s));
	
	MSB_DBG_PRINTF("[index]->hex  : binString, remarks\n");
	for(i = 0; i < s->m_buf_len; i++){
		b = 7;
		memset(option, 0, 64);
		for(mask = 0x01; mask > 0x00; mask = mask << 1){
			bit_str[b] = buf[i] & mask ? '1':'0';
			--b;
		}
		bit_str[8] = '\0';
		
		if(i == s->t_oct_index){
			sprintf(option, "current seek->%zd bit of this octet", s->t_oct_bit);
		}
		
		MSB_DBG_PRINTF("[%05zd]->0x%02X : %s, %s\n", i, buf[i], bit_str, option);
	}
	if(s->t_oct_index == s->m_buf_len){
		MSB_DBG_PRINTF("current seek->END OF BUFFER\n");
	}
	MSB_DBG_PRINTF("###debug output of msb_cursor_t[%p]...THE END\n", s);
}


