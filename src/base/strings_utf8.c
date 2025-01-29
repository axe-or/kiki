#include "base.h"
#include "strings.h"

#define UTF8_RANGE1 ((I32)0x7f)
#define UTF8_RANGE2 ((I32)0x7ff)
#define UTF8_RANGE3 ((I32)0xffff)
#define UTF8_RANGE4 ((I32)0x10ffff)

#define UTF16_SURROGATE1 ((I32)0xd800)
#define UTF16_SURROGATE2 ((I32)0xdfff)

#define UTF8_MASK2 (0x1f) /* 0001_1111 */
#define UTF8_MASK3 (0x0f) /* 0000_1111 */
#define UTF8_MASK4 (0x07) /* 0000_0111 */

#define UTF8_MASKX (0x3f) /* 0011_1111 */

#define UTF8_SIZE2 (0xc0) /* 110x_xxxx */
#define UTF8_SIZE3 (0xe0) /* 1110_xxxx */
#define UTF8_SIZE4 (0xf0) /* 1111_0xxx */

#define CONT (0x80)  /* 10xx_xxxx */

static inline
bool is_continuation_byte(Rune c){
	static const Rune CONTINUATION1 = 0x80;
	static const Rune CONTINUATION2 = 0xbf;
	return (c >= CONTINUATION1) && (c <= CONTINUATION2);
}

UTF8Encode utf8_encode(Rune c){
	UTF8Encode res = {0};

	if(is_continuation_byte(c) ||
	   (c >= UTF16_SURROGATE1 && c <= UTF16_SURROGATE2) ||
	   (c > UTF8_RANGE4))
	{
		return UTF8_ERROR_ENCODED;
	}

	if(c <= UTF8_RANGE1){
		res.len = 1;
		res.bytes[0] = c;
	}
	else if(c <= UTF8_RANGE2){
		res.len = 2;
		res.bytes[0] = UTF8_SIZE2 | ((c >> 6) & UTF8_MASK2);
		res.bytes[1] = CONT  | ((c >> 0) & UTF8_MASKX);
	}
	else if(c <= UTF8_RANGE3){
		res.len = 3;
		res.bytes[0] = UTF8_SIZE3 | ((c >> 12) & UTF8_MASK3);
		res.bytes[1] = CONT  | ((c >> 6) & UTF8_MASKX);
		res.bytes[2] = CONT  | ((c >> 0) & UTF8_MASKX);
	}
	else if(c <= UTF8_RANGE4){
		res.len = 4;
		res.bytes[0] = UTF8_SIZE4 | ((c >> 18) & UTF8_MASK4);
		res.bytes[1] = CONT  | ((c >> 12) & UTF8_MASKX);
		res.bytes[2] = CONT  | ((c >> 6)  & UTF8_MASKX);
		res.bytes[3] = CONT  | ((c >> 0)  & UTF8_MASKX);
	}
	return res;
}

static const UTF8Decode DECODE_ERROR = { .codepoint = UTF8_ERROR, .len = 1 };

UTF8Decode utf8_decode(U8 const* buf, Size len){
	UTF8Decode res = {0};
	if(buf == NULL || len <= 0){ return DECODE_ERROR; }

	U8 first = buf[0];

	if((first & CONT) == 0){
		res.len = 1;
		res.codepoint |= first;
	}
	else if ((first & ~UTF8_MASK2) == UTF8_SIZE2 && len >= 2){
		res.len = 2;
		res.codepoint |= (buf[0] & UTF8_MASK2) << 6;
		res.codepoint |= (buf[1] & UTF8_MASKX) << 0;
	}
	else if ((first & ~UTF8_MASK3) == UTF8_SIZE3 && len >= 3){
		res.len = 3;
		res.codepoint |= (buf[0] & UTF8_MASK3) << 12;
		res.codepoint |= (buf[1] & UTF8_MASKX) << 6;
		res.codepoint |= (buf[2] & UTF8_MASKX) << 0;
	}
	else if ((first & ~UTF8_MASK4) == UTF8_SIZE4 && len >= 4){
		res.len = 4;
		res.codepoint |= (buf[0] & UTF8_MASK4) << 18;
		res.codepoint |= (buf[1] & UTF8_MASKX) << 12;
		res.codepoint |= (buf[2] & UTF8_MASKX) << 6;
		res.codepoint |= (buf[3] & UTF8_MASKX) << 0;
	}
	else {
		return DECODE_ERROR;
	}

	// Validation step
	if(res.codepoint >= UTF16_SURROGATE1 && res.codepoint <= UTF16_SURROGATE2){
		return DECODE_ERROR;
	}
	if(res.len > 1 && !is_continuation_byte(buf[1])){
		return DECODE_ERROR;
	}
	if(res.len > 2 && !is_continuation_byte(buf[2])){
		return DECODE_ERROR;
	}
	if(res.len > 3 && !is_continuation_byte(buf[3])){
		return DECODE_ERROR;
	}

	return res;
}

// static inline
// bool is_continuation_byte

// Steps iterator forward and puts Rune and Length advanced into pointers,
// returns false when finished.
bool utf8_iter_next(UTF8Iterator* iter, UTF8Decode* out){
	if(iter->current >= iter->data_length){ return 0; }

	*out = utf8_decode(&iter->data[iter->current], iter->data_length - iter->current);

	iter->current += out->len;

	return 1;
}

// Steps iterator backward and puts Rune and its length into pointers,
// returns false when finished.
bool utf8_iter_prev(UTF8Iterator* iter, UTF8Decode* out){
	if(iter->current <= 0){ return false; }

	iter->current -= 1;
	while(is_continuation_byte(iter->data[iter->current])){
		iter->current -= 1;
	}

	*out = utf8_decode(&iter->data[iter->current], iter->data_length - iter->current);
	return true;
}

#undef CONT
