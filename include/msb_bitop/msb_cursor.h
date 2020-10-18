
#ifndef	MSB_CURSOR_H
#define	MSB_CURSOR_H

#define MSB_DBG_PRINTF printf

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>


/**
 *  \struct	msb_cursor_t
 *  \brief		Cursor structure for seeking bit array(uint8_t[]).
 *  \remarks	Defining 0th bit as MSB because this treats "bit ARRAY".
 */
typedef struct msb_cursor_s msb_cursor_t;
struct msb_cursor_s{
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

///Initiation bit cursor.
/**
 *  \fn			msb_cursor_init
 *  \brief		Initializing bitarray seeking cursor.
 *  
 *  \param		*s	Cursor instance.
 *  \return	Cursor instance.
 *  
 *  \remarks	This function not specify the buffer.
 */
msb_cursor_t	*msb_cursor_init(msb_cursor_t *s);

/**
 *  \fn				msb_cursor_load
 *  \brief			Specify the binary buffer.
 *  
 *  \param [in]	*s		Cursor instance
 *  \param [in]	*buf	Binary buffer you want to treat.
 *  \param [in]	buf_len	Length of *buf[bytes]
 *  \return		Total num of bits.
 *  
 *  \details シーク作業用の一時変数群はリセットされます
 */
size_t			msb_cursor_load(msb_cursor_t *s, unsigned char *buf, size_t buf_len);

///Getting length
/**
 *  \fn			msb_cursor_get_total_bytes
 *  \brief		Get the total byte length of buffer.
 *  
 *  \param [in]	*s		Cursor instance.
 *  \return	Total num of bytes.
 *  
 */
size_t			msb_cursor_get_total_bytes(msb_cursor_t *s);

/**
 *  \fn			msb_cursor_get_total_bits
 *  \brief		Get the total bit length of buffer.
 *  
 *  \param [in]	*s		Cursor instance.
 *  \return	Total num of bits.
 *  
 */
size_t			msb_cursor_get_total_bits(msb_cursor_t *s);

///Data manipulating API with ansolute pointing.
/**
 *  \brief			Read as uint32 data from directly specified digit.
 *  
 *  \param [in]	*s			Cursor instance
 *  \param [in]	onset_bit	Onset bit.
 *  \param [in]	width_bit	Width bit from Onset bit.
 *  \return		Read value. as uint32_t.(If you want as other value, cast it for now).
 *  \remarks		This read binary data in Big Endian.
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

