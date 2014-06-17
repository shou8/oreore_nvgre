#include <stdio.h>
#include <stdint.h>



typedef struct _nvgre_hdr {
	union {
		uint8_t byte;
		struct _bits {
			uint8_t : 4;
			uint8_t s: 1;
			uint8_t k: 1;
			uint8_t : 1;
			uint8_t c: 1;
		} bits;
	} flags;
	uint8_t ver;
	uint16_t protocol_type;
	uint8_t vsid;
	uint8_t flowid;
} nvgre_hdr;



void test1(void);
void test2(void);



int main(int argc, char *argv[])
{
//	test1();
	test2();

    return 0;
}



void test1(void)
{
	nvgre_hdr hdr;

	hdr.flags.byte = 0x00;
	hdr.flags.bits.c = 0;
	hdr.flags.bits.k = 1;
	hdr.flags.bits.s = 0;

	printf("c   : %X\n", hdr.flags.bits.c);
	printf("k   : %X\n", hdr.flags.bits.k);
	printf("s   : %X\n", hdr.flags.bits.s);
	printf("byte: %02X\n", hdr.flags.byte);
}



static const int bit_size = sizeof(uint8_t) * 8;



void test2(void)
{
	uint8_t x = 0x0F;
	uint8_t *bits = &x;

	char str[bit_size + 1];
	int bit = 1;
	int i = 0;

	str[bit_size+1] = '\0';

	for(i=bit_size; i>=0; i--, bit <<= 1)
		str[i] = (*bits & bit) ? '1' : '0';

	printf("%s\n", str);
}


