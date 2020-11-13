/**
 *	\file		msb_bitop.h
 *	\brief		8bit変数1個をmsb=0であれこれ操作するためのAPI
 *	\remarks	ビット配列として扱うまでもない場合にどうぞ
 */

#ifndef	MSB_BITOP_H
#define	MSB_BITOP_H

#include <stdint.h>


//「0ビット目からビットをn桁保全するか」のためのマスク。0桁保全はゼロ。
#define msbit_lmask(width) ((width > 7) ? (uint8_t)0xFF : (uint8_t)(~((uint8_t)0xFF >> width)))

//「nthビット目から、ビットをwidth桁保全する」ためのマスク
#define msbit_wmask(nth, width) ((uint8_t)msbit_lmask(width) >> nth)

//「7ビット目からビットをn桁保全するか」のためのマスク。0桁保全はゼロ。
#define msbit_rmask(width) ((width <= 0) ? (uint8_t)0x00 : (uint8_t)(((uint8_t)0xFF >> (8 - width))))

/**
 *	\def		msb_set_nth
 *	\brief		指定したビットを1にする
 *	\param [out]	ptr	(uint8_t*)操作したい変数のポインタ
 *	\param [in]	nth (integer) MSB=0とした、操作したいビット
 *	\remarks	
 */
#define	msb_set_nth(ptr, nth)	((uint8_t)((*ptr) |=  (1 << (7 - nth))))

/**
 *	\def		msb_clr_nth
 *	\brief		指定したビットを0にする
 *	\param [out]	ptr	(uint8_t*)操作したい変数のポインタ
 *	\param [in]	nth (integer) MSB=0とした、操作したいビット
 *	\remarks	
 */
#define	msb_clr_nth(ptr, nth)	((uint8_t)((*ptr) &= ~(1 << (7 - nth))))

/**
 *	\def		msb_inv_nth
 *	\brief		指定したビットを反転させる
 *	\param [out]	ptr	(uint8_t*)操作したい変数のポインタ
 *	\param [in]	nth (integer) MSB=0とした、操作したいビット
 *	\remarks	
 */
#define	msb_inv_nth(ptr, nth)	((uint8_t)((*ptr) ^=  (1 << (7 - nth))))

/**
 *	\def		msb_get_nth
 *	\brief		指定したビットの値を取得する
 *	\param [out]	ptr	(uint8_t*)操作したい変数のポインタ
 *	\param [in]	nth (integer) MSB=0とした、操作したいビット
 *	\remarks	
 */
#define	msb_get_nth(ptr, nth)	(((uint8_t)((*ptr) &   (1 << (7 - nth)))) != 0)

/**
 *	\def		msb_overwrite_nth
 *	\brief		指定した位置から、指定した幅の分だけ値を上書きする
 *	\param [out]	ptr	(uint8_t*)操作したい変数のポインタ
 *	\param [in]	nth (integer) MSB=0とした、操作したいビット
 *	\param [in]	width (integer) 書き込みたいビット幅。0は無効試合。最大8ビット
 *	\param [in]	val (uint8_t) 書き込みたい値。
 *	\remarks	
 */
#define msb_overwrite_nth(ptr, nth, width, val)	\
	do{\
		if(width == 1){\
			if(val & 0x01){\
				msb_set_nth(ptr, nth);\
			}\
			else{\
				msb_clr_nth(ptr, nth);\
			}\
			break;\
		}\
		uint8_t answer = (uint8_t)((*ptr) & (~msbit_wmask(nth, width)));\
		answer |= (uint8_t)(((val & msbit_rmask(width)) << (8 - width - nth)));\
		*ptr = answer;\
	}while(0)\

/**
 *	\def		msb_read_nth
 *	\brief		指定した位置から、指定した幅の分だけ値を上書きする
 *	\param [out]	ptr	(uint8_t*)読みたい変数のポインタ
 *	\param [in]	nth (integer) MSB=0とした、起点ビット
 *	\param [in]	width(integer) 読みたいビット幅。0は無効試合
 *	\remarks	
 */
#define msb_read_nth(ptr, nth, width) (width == 1) ? msb_get_nth(ptr, nth) : ((uint8_t)((*ptr) & (msbit_wmask(nth, width))) >> (8 - (width + nth)))

#endif	/* !MSB_BITOP_H */
 