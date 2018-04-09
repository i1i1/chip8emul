#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include <SDL2/SDL.h>


#ifdef DEBUG
	#define	dprintf(...)	printf(__VA_ARGS__)
#else
	#define	dprintf(...)
#endif

#define FREQ		400
#define PIXELSZ		16


#define	SURW	(64 * PIXELSZ)
#define	SURH	(32 * PIXELSZ)


SDL_Event event;
SDL_Window *window;
SDL_Renderer *renderer;



uint8_t display[64 * 32];
uint8_t key;

uint32_t freq;

SDL_TimerID sound_timer;
int32_t delay_timer;

uint8_t mem[0x1000];

uint16_t stack[100] = {0};
int sp = 0;

struct cpu {
	uint8_t V[0x10];

	uint16_t I;
	uint16_t PC;
} reg;


void
initgfx()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Init(SDL_INIT_TIMER);
	SDL_CreateWindowAndRenderer(SURW, SURH, 0, &window, &renderer);

	SDL_RenderPresent(renderer);
}

void
deinitgfx()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	exit(0);
}

int
pressed(uint8_t key)
{
	const uint8_t *state = SDL_GetKeyboardState(NULL);

	switch (key) {
		case 0x0:
			return state[SDL_SCANCODE_X];
		case 0x1:
			return state[SDL_SCANCODE_1];
		case 0x2:
			return state[SDL_SCANCODE_2];
		case 0x3:
			return state[SDL_SCANCODE_3];
		case 0x4:
			return state[SDL_SCANCODE_Q];
		case 0x5:
			return state[SDL_SCANCODE_W];
		case 0x6:
			return state[SDL_SCANCODE_E];
		case 0x7:
			return state[SDL_SCANCODE_A];
		case 0x8:
			return state[SDL_SCANCODE_S];
		case 0x9:
			return state[SDL_SCANCODE_D];
		case 0xA:
			return state[SDL_SCANCODE_Z];
		case 0xB:
			return state[SDL_SCANCODE_C];
		case 0xC:
			return state[SDL_SCANCODE_4];
		case 0xD:
			return state[SDL_SCANCODE_R];
		case 0xE:
			return state[SDL_SCANCODE_F];
		case 0xF:
			return state[SDL_SCANCODE_V];
		default:
			return 0;
	}
}

void
events()
{
	const uint8_t *state = SDL_GetKeyboardState(NULL);

	if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
		    deinitgfx();

	key = 0x10;

	if (state[SDL_SCANCODE_1])
		key = 0x1;
	else if (state[SDL_SCANCODE_2])
		key = 0x2;
	else if (state[SDL_SCANCODE_3])
		key = 0x3;
	else if (state[SDL_SCANCODE_4])
		key = 0xC;

	else if (state[SDL_SCANCODE_Q])
		key = 0x4;
	else if (state[SDL_SCANCODE_W])
		key = 0x5;
	else if (state[SDL_SCANCODE_E])
		key = 0x6;
	else if (state[SDL_SCANCODE_R])
		key = 0xD;

	else if (state[SDL_SCANCODE_A])
		key = 0x7;
	else if (state[SDL_SCANCODE_S])
		key = 0x8;
	else if (state[SDL_SCANCODE_D])
		key = 0x9;
	else if (state[SDL_SCANCODE_F])
		key = 0xE;

	else if (state[SDL_SCANCODE_Z])
		key = 0xA;
	else if (state[SDL_SCANCODE_X])
		key = 0x0;
	else if (state[SDL_SCANCODE_C])
		key = 0xB;
	else if (state[SDL_SCANCODE_V])
		key = 0xF;

	else if (state[SDL_SCANCODE_P]) {
		if (freq < 1000)
			freq += 1;
		printf("freq = %d\n", freq);
	}
	else if (state[SDL_SCANCODE_O]) {
		if (freq > 50)
			freq -= 1;
		printf("freq = %d\n", freq);
	}
}

uint32_t
soundcallback(uint32_t interval, void *p)
{
	SDL_RemoveTimer(sound_timer);

	fprintf(stderr, "BEEP!\n");

	return interval;
}

void
draw_pixel(uint8_t x, uint8_t y)
{
	if (x >= 64 || y >= 32)
		return;

	display[y * 64 + x] = (display[y * 64 + x] ? 0 : 1);

	if (!display[y * 64 + x])
		reg.V[0xF] = 1;
}

void
update()
{
	uint8_t x, y, i, j;

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

	for (x = 0; x < 64; x++)
		for (y = 0; y < 32; y++)
			if (display[y * 64 + x])
				for (i = 0; i < PIXELSZ; i++)
					for (j = 0; j < PIXELSZ; j++)
						SDL_RenderDrawPoint(renderer, x * PIXELSZ + i, y * PIXELSZ + j);

	SDL_RenderPresent(renderer);
}

void
draw(uint16_t addr, uint8_t x, uint8_t y, uint8_t l)
{
	int i, j;

	reg.V[0xF] = 0;

	for (i = 0; i < l; i++)
		for (j = 0; j < 8; j++)
			if (mem[addr + i] & (1 << j))
				draw_pixel(x + 7 - j, y + i);

	update();
}

void
load(FILE *fp, uint8_t *base)
{
	int tmp, i;

	for (i = 0x200; (tmp = getc(fp)) != EOF && i < 0x1000; i++)
		mem[i] = (uint8_t)tmp;
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

void mainloop();

int
main(int argc, char *argv[])
{
	FILE *fp;

	if (argc < 2 || !(fp = fopen(argv[1], "r")))
		exit(1);

	load(fp, mem);
	fontinit();
	initgfx();
	srand(time(NULL));
	freq = FREQ;

	mainloop();

	deinitgfx();

	return 0;
}

void
iter()
{
	uint16_t ci, i;
	int16_t tmp;

	ci = mem[reg.PC] << 8 | mem[reg.PC + 1];
	reg.PC += 2;

	switch (ci & 0xF000) {
		case 0x0000:
			if (ci == 0x00E0) {
				dprintf("%03x:%04x	CLEAR DISPLAY\n", reg.PC, ci);

				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				SDL_RenderClear(renderer);
			}
			else if (ci == 0x00EE) {
				dprintf("%03x:%04x	RETURN\n", reg.PC, ci);

				if (!sp)
					exit(0);

				reg.PC = stack[--sp];
			}
			else if ((0x00C0 <= ci && ci <= 0x00CF) ||
					(0x00FB <= ci && ci <= 0x00FF)){
				fprintf(stderr, "Probably chip-48 or super chip 8 rom.\n");
				fprintf(stderr, "Exiting\n");
				exit(1);
			}
			else
				dprintf("%03x:%04x	CALL RCA 1802 AT %03x; IGNORED\n",
					reg.PC, ci, ci & 0xfff);
			break;
		case 0x1000:
			dprintf("%03x:%04x	JUMP TO %03x\n", reg.PC, ci,
					ci & 0xFFF);

			reg.PC = ci & 0xFFF;

			break;
		case 0x2000:
			dprintf("%03x:%04x	CALL AT %03x\n", reg.PC, ci,
					ci & 0xFFF);

			stack[sp++] = reg.PC;
			reg.PC = ci & 0xFFF;

			break;
		case 0x3000:
			dprintf("%03x:%04x	if (V%x == %02x) skip\n",
					reg.PC, ci, (ci >> 8) & 0xF, ci & 0xFF);

			if (reg.V[(ci >> 8) & 0xF] == (ci & 0xFF))
				reg.PC += 2;

			break;
		case 0x4000:
			dprintf("%03x:%04x	if (V%x != %02x) skip\n",
					reg.PC, ci, (ci >> 8) & 0xF, ci & 0xFF);

			if (reg.V[(ci >> 8) & 0xF] != (ci & 0xFF))
				reg.PC += 2;

			break;
		case 0x5000:
			dprintf("%03x:%04x	if (V%x == V%x) skip\n",
					reg.PC, ci, (ci >> 8) & 0xF, (ci >> 4) & 0xF);

			if (reg.V[(ci >> 8) & 0xF] == reg.V[(ci >> 4) & 0xF])
				reg.PC += 2;

			break;
		case 0x6000:
			dprintf("%03x:%04x	V%x = %02x\n",
					reg.PC, ci, (ci >> 8) & 0xF, ci & 0xFF);

			reg.V[(ci >> 8) & 0xF] = ci & 0xFF;

			break;
		case 0x7000:
			dprintf("%03x:%04x	V%x += %02x\n",
					reg.PC, ci, (ci >> 8) & 0xF, ci & 0xFF);

			reg.V[(ci >> 8) & 0xF] += (ci & 0xFF);

			break;
		case 0x8000:
			switch (ci & 0xF) {
				case 0x0:
					dprintf("%03x:%04x	V%x = V%x\n",
						reg.PC, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF);
					reg.V[(ci >> 8) & 0xF] = reg.V[(ci >> 4) & 0xF];
					break;
				case 0x1:
					dprintf("%03x:%04x	V%x |= V%x\n",
						reg.PC, ci, (ci >> 8) & 0xF, (ci >> 4) & 0xF);
					reg.V[(ci >> 8) & 0xF] |= reg.V[(ci >> 4) & 0xF];
					break;
				case 0x2:
					dprintf("%03x:%04x	V%x &= V%x\n",
						reg.PC, ci, (ci >> 8) & 0xF, (ci >> 4) & 0xF);
					reg.V[(ci >> 8) & 0xF] &= reg.V[(ci >> 4) & 0xF];
					break;
				case 0x3:
					dprintf("%03x:%04x	V%x ^= V%x\n",
						reg.PC, ci, (ci >> 8) & 0xF, (ci >> 4) & 0xF);
					reg.V[(ci >> 8) & 0xF] ^= reg.V[(ci >> 4) & 0xF];
					break;
				case 0x4:
					dprintf("%03x:%04x	V%x += V%x\n",
						reg.PC, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF);
					tmp = reg.V[(ci >> 8) & 0xF] + reg.V[(ci >> 4) & 0xF];

					if (tmp > 255)
						reg.V[0xF] = 1;
					else
						reg.V[0xF] = 0;

					reg.V[(ci >> 8) & 0xF] += reg.V[(ci >> 4) & 0xF];

					break;
				case 0x5:
					dprintf("%03x:%04x	V%x -= V%x\n",
						reg.PC, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF);
					if (reg.V[(ci >> 8) & 0xF] >= reg.V[(ci >> 4) & 0xF])
						reg.V[0xF] = 1;
					else
						reg.V[0xF] = 0;

					reg.V[(ci >> 8) & 0xF] -= reg.V[(ci >> 4) & 0xF];
					break;
				case 0x6:
					dprintf("%03x:%04x	V%x = V%x = V%x >> 1\n",
						reg.PC, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF, (ci >> 4) & 0xF);
					reg.V[0xF] = reg.V[(ci >> 4) & 0xF] % 2;
					reg.V[(ci >> 4) & 0xF] >>= 1;
					reg.V[(ci >> 8) & 0xF] = reg.V[(ci >> 4) & 0xF];
					break;
				case 0x7:
					dprintf("%03x:%04x	V%x = V%x - V%x\n",
						reg.PC, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF, (ci >> 8) & 0xF);
					if (reg.V[(ci >> 4) & 0xF] >= reg.V[(ci >> 8) & 0xF])
						reg.V[0xF] = 1;
					else
						reg.V[0xF] = 0;

					reg.V[(ci >> 8) & 0xF] = reg.V[(ci >> 4) & 0xF] - reg.V[(ci >> 8) & 0xF];
					break;
				case 0xE:
					dprintf("%03x:%04x	V%x = V%x = V%x << 1\n",
						reg.PC, ci, (ci >> 8) & 0xF,
						(ci >> 4) & 0xF, (ci >> 4) & 0xF);
					reg.V[0xF] = reg.V[(ci >> 4) & 0xF] >> 7;
					reg.V[(ci >> 4) & 0xF] <<= 1;
					reg.V[(ci >> 8) & 0xF] = reg.V[(ci >> 4) & 0xF];
					break;
				default:
					dprintf("ERR 1\n");
					exit(1);
			}
			break;
		case 0x9000:
			dprintf("%03x:%04x	if (V%x != V%x) skip \n",
					reg.PC, ci, (ci >> 8) & 0xF, (ci >> 4) & 0xFF);

			if (reg.V[(ci >> 8) & 0xF] != reg.V[(ci >> 4) & 0xF])
				reg.PC += 2;

			break;
		case 0xA000:
			dprintf("%03x:%04x	I = %03x\n",
					reg.PC, ci, ci & 0xFFF);

			reg.I = ci & 0xFFF;

			break;
		case 0xB000:
			dprintf("%03x:%04x	PC = V0 + %03x\n",
					reg.PC, ci, ci & 0xFFF);
			reg.PC = reg.V[0] + (ci & 0xFFF);

			break;
		case 0xC000:
			dprintf("%03x:%04x	V%x = rand() & %02x\n",
					reg.PC, ci, (ci >> 8) & 0xF, ci & 0xFF);

			reg.V[(ci >> 8) & 0xF] = rand() & (ci & 0xFF);

			break;
		case 0xD000:
			dprintf("%03x:%04x	draw(V%x, V%x, %x); NOT COMPLETE\n",
					reg.PC, ci, (ci >> 8) & 0xF,
					(ci >> 4) & 0xF, ci & 0xF);

			draw(reg.I, reg.V[(ci >> 8) & 0xF], reg.V[(ci >> 4) & 0xF], ci & 0xF);

			break;
		case 0xE000:
			switch (ci & 0xFF) {
				case 0x9E:
					dprintf("%03x:%04x	if (key() == V%x) skip\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					if (pressed(reg.V[(ci >> 8) & 0xF]))
						reg.PC += 2;

					break;
				case 0xA1:
					dprintf("%03x:%04x	if (key() != V%x) skip\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					if (!pressed(reg.V[(ci >> 8) & 0xF]))
						reg.PC += 2;
					break;
				default:
					dprintf("ERR 2\n");
					exit(1);
			}
			break;
		case 0xF000:
			switch (ci & 0xFF) {
				case 0x07:
					dprintf("%03x:%04x	V%x = get_delay()\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					if (delay_timer > 0)
						reg.V[(ci >> 8) & 0xF] = delay_timer / 60;
					else
						reg.V[(ci >> 8) & 0xF] = 0;

					break;
				case 0x0A:
					dprintf("%03x:%04x	V%x = get_key()\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					reg.V[(ci >> 8) & 0xF] = key;
					break;
				case 0x15:
					dprintf("%03x:%04x	delay_timer(V%x)\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					delay_timer = reg.V[(ci >> 8) & 0xF] * 1000 / 60;
					break;
				case 0x18:
					dprintf("%03x:%04x	sound_timer(V%x)\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					sound_timer = SDL_AddTimer(reg.V[(ci >> 8) & 0xF] * 1000 / 60, soundcallback, NULL);
					break;
				case 0x1E:
					dprintf("%03x:%04x	I += V%x\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					reg.I += reg.V[(ci >> 8) & 0xF];
					break;
				case 0x29:
					dprintf("%03x:%04x	I = sprite[V%x]\n", reg.PC, ci, (ci >> 8) & 0xF);
					reg.I = 0x50 + 5 * reg.V[(ci >> 8) & 0xF];
					dprintf("I = %x\n", reg.I);
					break;
				case 0x33:
					dprintf("%03x:%04x	BCD(V%x)\n",
							reg.PC, ci, (ci >> 8) & 0xF);
					mem[reg.I + 2] = reg.V[(ci >> 8) & 0xF] % 10;
					mem[reg.I + 1] = reg.V[(ci >> 8) & 0xF] / 10 % 10;
					mem[reg.I] = reg.V[(ci >> 8) & 0xF] / 100;
					break;
				case 0x55:
					dprintf("%03x:%04x	reg_dump(V%x, I)\n",
							reg.PC, ci, (ci >> 8) & 0xF);

					for (i = 0; i <= ((ci >> 8) & 0xF); i++)
						mem[reg.I + i] = reg.V[i];

					break;
				case 0x65:
					dprintf("%03x:%04x	reg_load(V%x, I)\n",
							reg.PC, ci, (ci >> 8) & 0xF);

					for (i = 0; i <= ((ci >> 8) & 0xF); i++)
						reg.V[i] = mem[reg.I + i];

					break;
				default:
					dprintf("%03x:%04x	ERR\n", reg.PC, ci);

					//exit(1);
			}
	}
}

void
mainloop()
{
	uint32_t time;
	int i;

	reg.PC = 0x200;
	key = 0x10;

	for (;;) {
		time = SDL_GetTicks();

		for (i = 0; i < 8; i++) {
			events();
			iter();
		}

		time = time - SDL_GetTicks();

		if (delay_timer > 0)
			delay_timer -= 1000 / freq * i;

		usleep(1000000 / freq * i - time * 1000);
	}
}

