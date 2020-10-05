#include <msb_bitop/msb_cursor.h>
#include <msb_bitop/msb_bitop.h>
#include <stdio.h>
#include <string.h>

#define	BUFLEN 32


//コンセプト：ビット配列をビット配列として扱い、読み出すことができるようにする

//ハッシュ関数および比較関数の実装

int main(int argc, char *argv[]){
	
	uint8_t raw_array[BUFLEN];
	uint8_t	new_array[BUFLEN];
	
	int i;
	
	int read_bit;
	
	msb_cursor_t	sample, for_new;;
	memset(&sample, 0, sizeof(msb_cursor_t));
	memset(raw_array, 0, BUFLEN);
	memset(new_array, 0, BUFLEN);
	msb_cursor_t	*s = msb_cursor_init(&sample);
	msb_cursor_t	*n = msb_cursor_init(&for_new);
	size_t			remain_bits;
	
	//扱いたいバッファを設定する
	msb_cursor_load(s, raw_array, BUFLEN);
	
	//9ビット進める
	msb_cursor_seek_nbits(s, 9);
	printf("seek %d bits\n", 9);
	printf("seek point:%zd\n", msb_cursor_get_bit_index(s));
	
	//ゼロ起点で9ビット目から6ビット分だけ、0x0F(b001111)を書き込む
	msb_cursor_overwrite_nth_uint32(s, 9, 6, 0x0F);
	
	//ゼロ起点で21ビット目から28ビット分だけ書き込む。その際先頭4ビットをはみ出すように書く
	//よって、書かれるのはFAFBFCF0(末尾0省略)
	msb_cursor_overwrite_nth_uint32(s, 21, 28, 0x8FAFBFCF);
	
	//追加で7ビット進める（シークは16ビット目）
	msb_cursor_seek_nbits(s, 7);
	printf("seek %d bits\n", 7);
	printf("seek point:%zd\n", msb_cursor_get_bit_index(s));
	
	//3ビット少しずつ進める（シークは19ビット目）
	msb_cursor_seek_to_next_bit(s);
	msb_cursor_seek_to_next_bit(s);
	msb_cursor_seek_to_next_bit(s);
	printf("seek %d  bits 3 times\n", 1);
	printf("seek point:%zd \n", msb_cursor_get_bit_index(s));
	
	//バイト境界へ進む（16ビットの次なので24ビットに進める）
	//４回追加で繰り返しても何も起こらない
	msb_cursor_seek_to_byte_align(s);
	printf("seek to byte aligh\n");
	printf("seek point:%zd \n", msb_cursor_get_bit_index(s));
	
	msb_cursor_seek_to_byte_align(s);
	msb_cursor_seek_to_byte_align(s);
	msb_cursor_seek_to_byte_align(s);
	msb_cursor_seek_to_byte_align(s);
	printf("seek to byte aligh 4 times(no effect)\n");
	printf("seek point:%zd \n", msb_cursor_get_bit_index(s));
	
	//１バイトずつ進める。それを４回+α繰り返す（+32ビット+8ビット=64ビット）
	msb_cursor_seek_nbits(s, 3);	//あらかじめ３ビットシークするが、これは打ち消される
	msb_cursor_seek_to_next_byte(s);
	msb_cursor_seek_nbits(s, 3);
	msb_cursor_seek_to_next_byte(s);
	msb_cursor_seek_nbits(s, 3);
	msb_cursor_seek_to_next_byte(s);
	printf("seek %d  bytes(each after 3bits per byte: no effect)\n", 3);
	printf("seek point:%zd \n", msb_cursor_get_bit_index(s));
	
	msb_cursor_seek_nbits(s, 9);	//9ビット進める。8ビット以上進んだ状態になるので、
	printf("seek %d  bits\n", 9);
	msb_cursor_seek_to_next_byte(s);	//ここでは1ビット進んだ状態からのnext_byte: 1バイト(8 - 1 = 7bit)進む
	printf("seek to byte align\n");
	printf("seek point:%zd \n", msb_cursor_get_bit_index(s));
	
	//ゼロ起点で64ビット目から9ビット分だけ書き込む
	msb_cursor_overwrite_nth_uint32(s, 64, 9, 0x1FF);
	
	//ゼロ機転で31バイト目を1で満たす
	msb_cursor_overwrite_nth_uint32(s, 31*8, 8, 0xFF);
	
	//現在地[ビット]を取得
	printf("seek point:%zd\n", msb_cursor_get_bit_index(s));
	//そこから所定の幅だけ値を読み出す
	read_bit = 9;
	int32_t got_int = msb_cursor_read_uint32(s, read_bit);
	printf("read %d  bit from seekpoint->0x%02X(%d)\n", read_bit, got_int, got_int);
	
	printf("cursor seek writing test\n");
	
	//21バイト目の2ビット目に移動する
	msb_cursor_seek_to_nth_bit(s, 21*8 + 2);
	for(i = 0; i < 4; i++){
		//そこから、1ビット間隔をあけて4回ビットを打つ。
		//→22バイト目の0ビット目まで2ビット間隔で1が立つ。
		msb_cursor_overwrite_uint32(s, 1, 1);
		msb_cursor_seek_nbits(s, 2);
	}
	
	//最後に、次のバイト（23バイト目）まで歩を進め、そこに0xA2C1を書き込む。
	msb_cursor_seek_to_byte_align(s);
	msb_cursor_overwrite_uint32(s, 16, 0xA2C1);
	
	msb_cursor_dbg_print(s);
	
	
	//移行先のバッファを設定する
	msb_cursor_load(n, new_array, BUFLEN);
	
	//もともとのものを13ビット目にシークを戻す
	msb_cursor_seek_to_nth_bit(s, 13);
	remain_bits = msb_cursor_get_remain_bits(s);
	printf("read %zd bit to other buffer\n", remain_bits);
	msb_cursor_read_to_buf(s, remain_bits, new_array, BUFLEN-1);
	printf("seek to first: %zd\n", msb_cursor_seek_to_nth_bit(n, 0));
	printf("final seeked: %zd\n", msb_cursor_seek_to_nth_bit(n, BUFLEN * 8 + 234));
	printf("read from EOB: %u\n", msb_cursor_read_uint32(n, 32));
	printf("write to EOB\n");
	msb_cursor_overwrite_uint32(n, 32, 0xFF);
	msb_cursor_dbg_print(n);
	printf("seek -4 bit: %zd\n", msb_cursor_seek_nbits(n, -4));
	printf("seek -300 bit: %zd\n", msb_cursor_seek_nbits(n, -300));
	msb_cursor_dbg_print(n);
	
	//単一バッファオペレート。
	uint8_t oval, owidth, obit, obyte;
	oval = 0xAB;
	obit = 4;
	owidth = 4;
	obyte = 11;
	printf("write 0x%02X(%u bit) to %uth bit on %u byte\n", oval, owidth, obit, obyte);
	msb_overwrite_nth(&new_array[obyte], obit, owidth, oval);
	printf("wrote: 0x%02X\n", msb_read_nth(&new_array[obyte], 2, 1));
	msb_cursor_dbg_print(n);
	return 0;
}

