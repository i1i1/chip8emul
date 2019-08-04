#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "helpers.h"


#define FREQ		400
#define TIMER_FREQ	60

#define PIXELSZ		16
#define	SURW	(64 * PIXELSZ)
#define	SURH	(32 * PIXELSZ)



uint8_t display[64 * 32];
uint8_t key;

uint8_t mem[0x1000];

uint16_t stack[100] = {0};
int sp = 0;

struct cpu {
	uint8_t V[0x10];

	uint16_t I;
	uint16_t PC;

	volatile uint16_t DT;
	volatile uint16_t ST;
} reg;


int
get_key(void)
{
	for (int i = 0; i < 0x10; i++) {
		if (helper_is_key_pressed(i))
			return i;
	}
	return 0x10;
}

void
set_pixel(uint8_t x, uint8_t y)
{
	if (x >= 64 || y >= 32)
		return;

	display[y * 64 + x] = (display[y * 64 + x] ? 0 : 1);

	if (!display[y * 64 + x])
		reg.V[0xF] = 1;
}

void
draw_pixel(uint8_t x,uint8_t y)
{
	for (int i = 0; i < PIXELSZ; i++) {
		for (int j = 0; j < PIXELSZ; j++)
			helper_draw_point(x * PIXELSZ + i, y * PIXELSZ + j);
	}
}

void
update_screen()
{
	uint8_t x, y;

	helper_clear_screen(0, 0, 0, 0);
	helper_set_draw_color(255, 255, 255, 0);

	for (x = 0; x < 64; x++) {
		for (y = 0; y < 32; y++) {
			if (display[y * 64 + x])
				draw_pixel(x, y);
		}
	}

	helper_update_screen();
}

void
draw(uint16_t addr, uint8_t x, uint8_t y, uint8_t l)
{
	int i, j;

	reg.V[0xF] = 0;

	for (i = 0; i < l; i++) {
		for (j = 0; j < 8; j++) {
			if (mem[addr + i] & (1 << j))
				set_pixel(x + 7 - j, y + i);
		}
	}

	update_screen();
}

void
load(FILE *fp)
{
	size_t _ = fread(mem + 0x200, 0x800, 1, fp);
	(void)_;
}

void
fontinit()
{
	uint8_t font[0x50] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	memcpy(mem + 0x50, font, 0x50);
}

void
timers_callback(void)
{
	if (reg.DT > 0)
		reg.DT--;
	if (reg.ST > 0)
		reg.ST--;

	helper_beep(reg.ST);
}

void mainloop();

int
main(int argc, char *argv[])
{
	FILE *fp;

	if (argc < 2 || !(fp = fopen(argv[1], "r")))
		exit(1);

	srand(time(NULL));
	load(fp);
	fclose(fp);
	fontinit();
	helper_initgfx(SURW, SURH);
	helper_beep_init();
	helper_add_timer(MS_IN_S/TIMER_FREQ, timers_callback);

	mainloop();

	helper_beep_deinit();
	helper_deinitgfx();

	return 0;
}

void
iter()
{
	uint16_t ci, i;
	int16_t tmp;

	ci = mem[reg.PC] << 8 | mem[reg.PC + 1];
	reg.PC += 2;

	int op = ci >> 12;
	int args = ci & 0x0FFF;

	switch (op) {
	case 0x0:
		switch (ci) {
		case 0x00E0:
			helper_clear_screen(0, 0, 0, 0);
			break;
		case 0x00EE:
			if (!sp)
				exit(0);
			reg.PC = stack[--sp];
			break;
		default:
			printf("ERR 1 %4x\n", ci);
			exit(1);
		}
		break;
	case 0x1:
		reg.PC = args;
		break;
	case 0x2:
		stack[sp++] = reg.PC;
		reg.PC = args;
		break;
	case 0x3: {
		int rg  = (args >> 8) & 0xF;
		int val = args & 0x00FF;

		if (reg.V[rg] == val)
			reg.PC += 2;
		break;
	}
	case 0x4: {
		int rg   = (args >> 8) & 0xF;
		int val  = args & 0x00FF;

		if (reg.V[rg] != val)
			reg.PC += 2;
		break;
	}
	case 0x5: {
		int rx = (args >> 8) & 0xF;
		int ry = (args >> 4) & 0xF;

		if (reg.V[rx] == reg.V[ry])
			reg.PC += 2;
		break;
	}
	case 0x6: {
		int rg = (args >> 8) & 0xF;
		int val = args & 0x00FF;

		reg.V[rg] = val;
		break;
	}
	case 0x7: {
		int rg = (args >> 8) & 0xF;
		int val = args & 0x00FF;

		reg.V[rg] += val;
		break;
	}
	case 0x8: {
		int type = (args >> 0) & 0xF;
		int ry   = (args >> 4) & 0xF;
		int rx   = (args >> 8) & 0xF;

		switch (type) {
		case 0x0:
			reg.V[rx] = reg.V[ry];
			break;
		case 0x1:
			reg.V[rx] |= reg.V[ry];
			break;
		case 0x2:
			reg.V[rx] &= reg.V[ry];
			break;
		case 0x3:
			reg.V[rx] ^= reg.V[ry];
			break;
		case 0x4:
			tmp = reg.V[rx] + reg.V[ry];

			if (tmp > 255)
				reg.V[0xF] = 1;
			else
				reg.V[0xF] = 0;

			reg.V[rx] += reg.V[ry];

			break;
		case 0x5:
			if (reg.V[rx] >= reg.V[ry])
				reg.V[0xF] = 1;
			else
				reg.V[0xF] = 0;

			reg.V[rx] -= reg.V[ry];
			break;
		case 0x6:
			reg.V[0xF] = reg.V[ry] & 1;
			reg.V[ry] >>= 1;
			reg.V[rx] = reg.V[ry];
			break;
		case 0x7:
			if (reg.V[ry] >= reg.V[rx])
				reg.V[0xF] = 1;
			else
				reg.V[0xF] = 0;

			reg.V[rx] = reg.V[ry] - reg.V[rx];
			break;
		case 0xE:
			reg.V[0xF] = reg.V[ry] >> 7;
			reg.V[ry] <<= 1;
			reg.V[rx] = reg.V[ry];
			break;
		default:
			printf("ERR 2\n");
			exit(1);
		}
		break;
	}
	case 0x9: {
		int rx = (args >> 8) & 0xF;
		int ry = (args >> 4) & 0xF;

		if (reg.V[rx] != reg.V[ry])
			reg.PC += 2;
		break;
	}
	case 0xA:
		reg.I = args;
		break;
	case 0xB:
		reg.PC = reg.V[0] + args;
		break;
	case 0xC: {
		int rg = (args >> 8) & 0xF;
		int val = args & 0x00FF;

		reg.V[rg] = rand() & val;
		break;
	}
	case 0xD: {
		int rx = (args >> 8) & 0xF;
		int ry = (args >> 4) & 0xF;
		int nib= (args >> 0) & 0xF;

		draw(reg.I, reg.V[rx], reg.V[ry], nib);
		break;
	}
	case 0xE: {
		int rx = (args >> 8) & 0xF;
		int type = args & 0xFF;

		switch (type) {
		case 0x9E:
			if (helper_is_key_pressed(reg.V[rx]))
				reg.PC += 2;

			break;
		case 0xA1:
			if (!helper_is_key_pressed(reg.V[rx]))
				reg.PC += 2;
			break;
		default:
			printf("ERR 3\n");
			exit(1);
		}
		break;
	}
	case 0xF: {
		int rx = (args >> 8) & 0xF;
		int type = args & 0xFF;

		switch (type) {
		case 0x07:
			reg.V[rx] = reg.DT;
			break;
		case 0x0A:
			reg.V[rx] = get_key();
			break;
		case 0x15:
			reg.DT = reg.V[rx];
			break;
		case 0x18:
			reg.ST = reg.V[rx];
			break;
		case 0x1E:
			reg.I += reg.V[rx];
			break;
		case 0x29:
			reg.I = 0x50 + 5 * reg.V[rx];
			break;
		case 0x33:
			mem[reg.I + 1] = reg.V[rx] / 10 % 10;
			mem[reg.I] = reg.V[rx] / 100;
			break;
		case 0x55:
			for (i = 0; i <= rx; i++)
				mem[reg.I + i] = reg.V[i];
			break;
		case 0x65:
			for (i = 0; i <= rx; i++)
				reg.V[i] = mem[reg.I + i];
			break;
		default:
			printf("ERR 4\n");
			exit(1);
		}
	}
	}
}

void
mainloop()
{
	uint32_t time;

	reg.PC = 0x200;
	key = 0x10;

	for (;;) {
		const int num_insts = 16;

		time = helper_time();

		if (helper_is_user_exit())
			return;

		for (int i = 0; i < num_insts; i++)
			iter();

		time = time - helper_time();
		helper_sleep(MS_IN_S/FREQ*num_insts - time);
	}
}

