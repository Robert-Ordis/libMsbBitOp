
#ifndef	MSB_CURSOR_H
#define	MSB_CURSOR_H

#define MSB_DBG_PRINTF printf

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

/**
 * MSBビット配列操作補助ライブラリ
 *
 */


/**
 *  \struct	msb_cursor_t
 *  \brief		文字列バッファをビット配列として、シークしながら読んでくための構造体
 *  \brief		「ビット配列」なのでMSBを0として数えます
 */
typedef struct msb_cursor_s{
	//取り扱う実データ群
	uint8_t			*m_buf;				//文字列バッファへのポインタ
	size_t			m_buf_len;			//扱っている文字列バッファの長さ
	
	//シークしながら作業するための一時変数群
	size_t			t_oct_bit;			//1オクテット中のシークインデックス（MSBを0、LSBを7として扱います）
	size_t			t_oct_index;		//現在シーク中のバイト（デフォルトはゼロ）
} msb_cursor_t;


#if defined(__cplusplus)
extern "C"
{
#endif

///初期化系列
/**
 *  \brief ビット配列の初期化
 *  
 *  \param		初期化したいビット配列。NULLの場合はmallocで生成
 *  \return	初期化したビット配列構造体
 *  
 *  \details この段階ではまだバッファを指定しません。
 */
msb_cursor_t	*msb_cursor_init(msb_cursor_t *s);

/**
 *  \brief 文字列バッファを設定する
 *  
 *  \param [in] s 扱うビット配列
 *  \param [in] buf 設定したい文字列。NULLを指定するとmalloc生成
 *  \param [in] buf_len 文字列バッファの長さ[bytes]。未指定に戻す場合は０を指定。
 *  \return 今回扱う文字列バッファのビット単位の長さ
 *  
 *  \details シーク作業用の一時変数群はリセットされます
 */
size_t			msb_cursor_load(msb_cursor_t *s, unsigned char *buf, size_t buf_len);

///長さ取得系列
/**
 *  \brief ビット配列の長さをバイト単位で取得する
 *  
 *  \param		対象のビット配列
 *  \return	現在扱っている配列長[bytes]
 *  
 */
size_t			msb_cursor_get_total_bytes(msb_cursor_t *s);

/**
 *  \brief ビット配列の長さをビット単位で取得する
 *  
 *  \param		対象のビット配列
 *  \return	現在扱っている配列長[bits]
 *  
 */
size_t			msb_cursor_get_total_bits(msb_cursor_t *s);

///位置を直接指定したデータ操作API
/**
 *  \brief 読み込み開始点を直接指定してint値を読み込む（符号なし）
 *  
 *  \param		s			対象のビット配列
 *  \param		onset_bit	読み込み開始地点のビット
 *  \param		width_bit	開始地点から読み込む幅（0を指定すると0が帰ります）
 *  \return	読み出した値
 *	\remarks	値はビッグエンディアン（MSB）で読み出されます。
 *  
 */
uint32_t		msb_cursor_read_nth_uint32(msb_cursor_t *s, size_t onset_bit, size_t width_bit);

/**
 *  \brief 読み込み開始点を直接指定してビットを取得する
 *  
 *  \param [in] s			対象のビット配列
 *  \param [in] onset_bit	読み込み開始地点のビット
 *  \param [in] width_bit	開始地点から読み込む幅（0を指定すると0が帰ります）
 *  \return	指定したビットの値
 *  
 *  \remarks	枠からはみ出した場合は-1を返します
 *  
 */
int				msb_cursor_read_nth_bit(msb_cursor_t *s, size_t onset_bit);

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
size_t			msb_cursor_read_nth_to_buf(msb_cursor_t *s, size_t onset_bit, size_t width_bit, uint8_t *buf, size_t buf_len);

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
void			msb_cursor_overwrite_nth_uint32(msb_cursor_t *s, size_t onset_bit, size_t width_bit, uint32_t value);



///シークしながら行う処理系統のためのAPI

/**
 *  \brief 現在シーク中のバイトを取得する
 *  
 *  \param [in] s 対象のビット配列
 *  \return 現在シーク中のバイトインデックス
 *  
 *  \details 例えば10ビット目＝1バイト+2ビット目の場合は0バイトを返す。
 */
size_t			msb_cursor_get_byte_index(msb_cursor_t *s);

/**
 *  \brief ビット単位での、現在のシークポイントを取得する
 *  
 *  \param [in] s 対象のビット配列
 *  \return ビット単位の現在のシークポイント（ゼロスタート）
 *  
 */
size_t			msb_cursor_get_bit_index(msb_cursor_t *s);

/**
 *  \brief ビット単位での残りシーク可能数を取得する
 *  
 *  \param [in] s 対象のビット配列
 *  \return あと何ビットシークできるか[bits]
 *  
 */
size_t			msb_cursor_get_remain_bits(msb_cursor_t *s);

/**
 *  \brief １ビットシークさせる
 *  
 *  \param [in] s 対象のビット配列
 *  \return 実際にシークできたビット数（０でシークできなかった）
 *  
 */
ssize_t			msb_cursor_seek_to_next_bit(msb_cursor_t *s);

/**
 *  \brief 現在地からNビットシークさせる
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] n シークさせるビット数
 *  \return 実際にシークできたビット数（０でシークできなかった）
 *  
 */
ssize_t			msb_cursor_seek_nbits(msb_cursor_t *s, ssize_t n);

/**
 *  \brief 指定したビットにシークさせる
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] nth シーク先のビット位置
 *  \return 実際にシークさせた場所
 *  
 *  \details 範囲外にシークさせた場合にはそれぞれの端にシークされます。逆に言えばnth=returnで正常シーク。
 */
ssize_t			msb_cursor_seek_to_nth_bit(msb_cursor_t *s, size_t nth);

/**
 *  \brief 現在のシーク位置から次のバイトへ進める
 *  
 *  \param [in] s 対象のビット配列
 *  \return 実際にシークしたビット数
 *  
 *  \details 0ビット目で呼んだ場合は8ビット進みます
 */
ssize_t			msb_cursor_seek_to_next_byte(msb_cursor_t *s);

/**
 *  \brief 現在のシーク位置からバイト境界へ進める
 *  
 *  \param [in] s 対象のビット配列
 *  \return 実際にシークしたビット数
 *  
 *  \details 1～7ビット目の場合は次のバイトへ、0ビット目でこれを呼んだ場合は0ビットシークされます。
 */
ssize_t			msb_cursor_seek_to_byte_align(msb_cursor_t *s);

/**
 *  \brief 現在のシーク位置からNビット指定した整数値を読み出す
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] width_bit 読み出したいビット幅
 *  \return 読み出した値
 *  
 *  \details はみ出した場合は右にゼロパディングした値を読み出します。
 */
uint32_t		msb_cursor_read_uint32(msb_cursor_t *s, size_t width_bit);

/**
 *  \brief 符号なし整数値を、現在のシーク位置からビット幅を指定して上書きする
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] width_bit 書き込む幅
 *  \param [in] value 書き込みたい値。マイナス値指定の場合は対象ビットをすべて1にします。
 *  
 *  \details 書き込む値に対してビット幅が足りない場合は左側（上側）が切り詰められます
 */
void			msb_cursor_overwrite_uint32(msb_cursor_t *s, size_t width_bit, uint32_t value);

/**
 *  \brief 現在のシーク位置から最後までのデータを指定バッファに書き込む
 *  
 *  \param [in] s 対象のビット配列
 *  \param [in] width_bit ビット幅
 *  \param [in] buf 書き込み先バッファ
 *  \param [in] buflen バッファの長さ[bytes]
 *  \return 読み出したバイト数
 *  
 *  \details もともとのバイト境界にかかわらず、データは左詰めで記述されます
 */
size_t			msb_cursor_read_to_buf(msb_cursor_t *s, size_t width_bit, uint8_t *buf, size_t buflen);


/**
 *  \brief デバッグ用print関数
 *  
 *  \param [in] s 対象のビット配列
 *  
 *  \details デバッグ用
 */
void			msb_cursor_dbg_print(msb_cursor_t *s);

#if defined(__cplusplus)
}
#endif

#endif	/* !MSB_CURSOR_H */

