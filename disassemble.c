#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


int si = 0;
uint16_t maxaddr = 0;

uint16_t
getinst(uint8_t *base, uint16_t addr)
{
	uint16_t res;

	if (addr < 0x200)
		printf("ERR at %x", addr);

	if (addr > maxaddr)
		exit(0);

	addr -= 0x200;
	res = (base[si] << 8) + base[si + 1];
	si += 2;

	return res;
}

void
load(FILE *fp, uint8_t *base)
{
	int tmp;

	while ((tmp = getc(fp)) != EOF)
		base[maxaddr++] = (uint8_t)tmp;

	maxaddr += 0x200;
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	uint16_t ci;
	uint8_t *base;
	uint16_t i;

	if (argc < 2)
		exit(1);

	fp = fopen(argv[1], "r");

	if (!fp || !(base = malloc(0xD00)))
		exit(1);

	load(fp, base);

	i = 0x200;

	while (1) {
		ci = getinst(base, i);

		if (ci == 0x00E0)
			printf("%03x:%04x	CLEAR DISPLAY\n", i, ci);
		else if (ci == 0x00EE)
			printf("%03x:%04x	RETURN\n", i, ci);
		else if (ci <= 0x1000)
			printf("%03x:%04x	CALL RCA 1802 AT %03x\n",
					i, ci, ci & 0xfff);
		else if (ci >= 0x1000 && ci < 0x2000)
			printf("%03x:%04x	JUMP TO %03x\n", i, ci,
					ci & 0xFFF);
		else if (ci >= 0x2000 && ci < 0x3000)
			printf("%03x:%04x	CALL AT %03x\n", i, ci,
					ci & 0xFFF);
		else if (ci >= 0x3000 && ci < 0x4000)
			printf("%03x:%04x	if (V%x == %02x) skip\n",
					i, ci, (ci >> 8) & 0xF, ci & 0xFF);
		else if (ci >= 0x4000 && ci < 0x5000)
			printf("%03x:%04x	if (V%x != %02x) skip\n",
					i, ci, (ci >> 8) & 0xF, ci & 0xFF);
		else if (ci >= 0x5000 && ci < 0x6000)
			printf("%03x:%04x	if (V%x == V%x) skip\n",
					i, ci, (ci >> 8) & 0xF, (ci >> 4) & 0xF);
		else if (ci >= 0x6000 && ci < 0x7000)
			printf("%03x:%04x	V%x = %02x\n",
					i, ci, (ci >> 8) & 0xF, ci & 0xFF);
		else if (ci >= 0x7000 && ci < 0x8000)
			printf("%03x:%04x	V%x += %02x\n",
					i, ci, (ci >> 8) & 0xF, ci & 0xFF);
		else if (ci >= 0x8000 && ci < 0x9000)
			switch (ci & 0xF) {
				case 0x0:
					printf("%03x:%04x	V%x = V%x\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF);
					break;
				case 0x1:
					printf("%03x:%04x	V%x = V%x | V%x\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 8) & 0xF, (ci >> 4) & 0xF);
					break;
				case 0x2:
					printf("%03x:%04x	V%x = V%x & V%x\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 8) & 0xF, (ci >> 4) & 0xF);
					break;
				case 0x3:
					printf("%03x:%04x	V%x = V%x ^ V%x\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 8) & 0xF, (ci >> 4) & 0xF);
					break;
				case 0x4:
					printf("%03x:%04x	V%x += V%x\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF);
					break;
				case 0x5:
					printf("%03x:%04x	V%x -= V%x\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF);
					break;
				case 0x6:
					printf("%03x:%04x	V%x = V%x = V%x >> 1\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF, (ci >> 4) & 0xF);
					break;
				case 0x7:
					printf("%03x:%04x	V%x = V%x - V%x << 1\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF, (ci >> 8) & 0xF);
					break;
				case 0xE:
					printf("%03x:%04x	V%x = V%x = V%x << 1\n",
						i, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF, (ci >> 4) & 0xF);
					break;
				default:
					printf("ERR 1\n");
					//exit(1);
			}
		else if (ci >= 0x9000 && ci < 0xA000)
			printf("%03x:%04x	if (V%x != V%x) skip \n",
					i, ci, (ci >> 8) & 0xF, (ci >> 4) & 0xFF);
		else if (ci >= 0xA000 && ci < 0xB000)
			printf("%03x:%04x	I = %03x\n",
					i, ci, ci & 0xFF);
		else if (ci >= 0xB000 && ci < 0xC000)
			printf("%03x:%04x	i = V0 + %03x\n",
					i, ci, ci & 0xFFF);
		else if (ci >= 0xC000 && ci < 0xD000)
			printf("%03x:%04x	V%x = rand() & %02x\n",
					i, ci, (ci >> 8) & 0xF, ci & 0xFF);
		else if (ci >= 0xD000 && ci < 0xE000)
			printf("%03x:%04x	draw(V%x, V%x, %x)\n",
					i, ci, (ci >> 8) & 0xF,
					(ci >> 4) & 0xF, ci & 0xF);
		else if (ci >= 0xE000 && ci < 0xF000)
			switch (ci & 0xFF) {
				case 0x9E:
					printf("%03x:%04x	if (ket() == V%x) skip\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0xA1:
					printf("%03x:%04x	if (ket() != V%x) skip\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				default:
					printf("ERR 2\n");
				//	exit(1);
			}
		else if (ci >= 0xF000)
			switch (ci & 0xFF) {
				case 0x07:
					printf("%03x:%04x	V%x = get_delay()\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x0A:
					printf("%03x:%04x	V%x = get_key()\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x15:
					printf("%03x:%04x	delay_timer(V%x)\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x18:
					printf("%03x:%04x	sound_timer(V%x)\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x1E:
					printf("%03x:%04x	I += V%x\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x29:
					printf("%03x:%04x	I = sprite[V%x]\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x33:
					printf("%03x:%04x	BCD(V%x); STORE(I)\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x55:
					printf("%03x:%04x	reg_dump(V%x, I)\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				case 0x65:
					printf("%03x:%04x	reg_load(V%x, I)\n",
							i, ci, (ci >> 8) & 0xF);
					break;
				default:
					printf("ERR 3\n");
					//exit(1);
			}
		else {
			printf("%03x:%04x	ERR or END\n", i, ci);
//			exit(1);
		}

		i += 2;
	}

	free(base);

	return 0;
}

