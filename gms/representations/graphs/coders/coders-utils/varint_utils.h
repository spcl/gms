
#ifndef PROJECT_VARINT_UTILS_H
#define PROJECT_VARINT_UTILS_H

#define TWO_POW_7 128
#define TWO_POW_14 16384
#define TWO_POW_20 1048576
#define TWO_POW_21 2097152
#define TWO_POW_28 268435456
#define TWO_POW_35 34359738368
#define TWO_POW_42 4398046511104
#define TWO_POW_49 562949953421312
#define TWO_POW_56 72057594037927936
#define TWO_POW_63 9223372036854775808

#define BIT_8_SET 0x80
#define BIT_8_CLEAR 0x7F

#define BITS_IN_BYTE 8
#define BYTES_IN_64BIT_WORD 8
#define BITS_IN_64BIT_WORD 64

#define NEXT_VAL_SMALLER 0x01
#define NEXT_VAL_GREATER 0x00

int toVarint(uint64_t x, unsigned char* out) {
    if(x == 0) {
        *out = 0;
        return 1;
    }

    if(x < TWO_POW_7) {
        *out = x & BIT_8_CLEAR;
        return 1;
    }
    else if(x >= TWO_POW_7 && x < TWO_POW_14) {
        *out = (x & BIT_8_CLEAR) | BIT_8_SET;
        *(out+1) = (x >> 7) & BIT_8_CLEAR;
        return 2;
    }
    else if(x >= TWO_POW_14 && x < TWO_POW_21) {
        *out = (x & BIT_8_CLEAR) | BIT_8_SET;
        *(out+1) = ((x >> 7) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+2) = ((x >> 14) & BIT_8_CLEAR);
        return 3;
    }
    else if(x >= TWO_POW_21 && x < TWO_POW_28) {
        *out = (x & BIT_8_CLEAR) | BIT_8_SET;
        *(out+1) = ((x >> 7) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+2) = ((x >> 14) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+3) = ((x >> 21) & BIT_8_CLEAR);
        return 4;
    }
    else if(x >= TWO_POW_28 && x < TWO_POW_35) {
        *out = (x & BIT_8_CLEAR) | BIT_8_SET;
        *(out+1) = ((x >> 7) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+2) = ((x >> 14) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+3) = ((x >> 21) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+4) = ((x >> 28) & BIT_8_CLEAR);
        return 5;
    }
    else if(x >= TWO_POW_35 && x < TWO_POW_42) {
        *out = (x & BIT_8_CLEAR) | BIT_8_SET;
        *(out+1) = ((x >> 7) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+2) = ((x >> 14) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+3) = ((x >> 21) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+4) = ((x >> 28) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+5) = ((x >> 35) & BIT_8_CLEAR);
        return 6;
    }
    else if(x >= TWO_POW_42 && x < TWO_POW_49) {
        *out = (x & BIT_8_CLEAR) | BIT_8_SET;
        *(out+1) = ((x >> 7) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+2) = ((x >> 14) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+3) = ((x >> 21) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+4) = ((x >> 28) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+5) = ((x >> 35) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+6) = ((x >> 42) & BIT_8_CLEAR);
        return 7;
    }
    else if(x >= TWO_POW_49 && x < TWO_POW_56) {
        *out = (x & BIT_8_CLEAR) | BIT_8_SET;
        *(out+1) = ((x >> 7) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+2) = ((x >> 14) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+3) = ((x >> 21) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+4) = ((x >> 28) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+5) = ((x >> 35) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+6) = ((x >> 42) & BIT_8_CLEAR) | BIT_8_SET;
        *(out+7) = ((x >> 49) & BIT_8_CLEAR);
        return 8;
    }

    std::cerr << "toVarint error" << std::endl;
    return -1;
}


int fromVarint(unsigned char* in, uint64_t* out) {
    unsigned char more = (*in) & BIT_8_SET;
    uint64_t val = (*in) & BIT_8_CLEAR;
    int next = 1;
    *out = val;

    while(more == BIT_8_SET) {
        ++in;
        more = (*in) & BIT_8_SET;
        val = (*in) & BIT_8_CLEAR;
        *out += (val << 7*next);
        ++next;
    }

    return next;
}

#endif //PROJECT_VARINT_UTILS_H
