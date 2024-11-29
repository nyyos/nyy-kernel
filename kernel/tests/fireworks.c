#include "ndk/kmem.h"
#include "ndk/sched.h"
#include <stdint.h>
#include <assert.h>
#include <limine.h>
#include <string.h>

#define BACKGROUND_COLOR 0x09090F

uint8_t *PixBuff;
int PixWidth, PixHeight, PixPitch, PixBPP;

void PlotPixel(uint32_t Color, int X, int Y)
{
	if (X < 0 || Y < 0 || X >= PixWidth || Y >= PixHeight)
		return;

	uint8_t *Place = PixBuff + PixPitch * Y + X * PixBPP / 8;

	switch (PixBPP / 8) {
	case 1:
		*Place = (uint8_t)(Color | (Color >> 8) | (Color >> 16));
		break;
	case 2:
		*((uint16_t *)Place) = Color | (Color >> 16);
		break;
	case 3:
		Place[0] = (uint8_t)Color;
		Place[1] = (uint8_t)(Color >> 8);
		Place[2] = (uint8_t)(Color >> 16);
		break;
	case 4:
		*((uint32_t *)Place) = Color;
		break;
	default:
		assert(!"Oh no!");
	}
}

void FillScreen(uint32_t Color)
{
	for (int Y = 0; Y < PixHeight; Y++) {
		switch (PixBPP / 8) {
		case 1: {
			uint8_t Value =
				(uint8_t)(Color | (Color >> 8) | (Color >> 16));
			uint8_t *Place = PixBuff + PixPitch * Y;
			for (int X = 0; X < PixWidth; X++) {
				*Place = Value;
				Place++;
			}
			break;
		}
		case 2: {
			uint16_t Value = (uint16_t)(Color | (Color >> 16));
			uint16_t *Place = (uint16_t *)(PixBuff + PixPitch * Y);
			for (int X = 0; X < PixWidth; X++) {
				*Place = Value;
				Place++;
			}
			break;
		}
		case 3: {
			uint8_t *Place = PixBuff + PixPitch * Y;
			for (int X = 0; X < PixWidth; X++) {
				Place[0] = (uint8_t)Color;
				Place[1] = (uint8_t)(Color >> 8);
				Place[2] = (uint8_t)(Color >> 16);
				Place += 3;
			}
			break;
		}
		case 4: {
			uint32_t *Place = (uint32_t *)(PixBuff + PixPitch * Y);
			for (int X = 0; X < PixWidth; X++) {
				*Place = Color;
				Place++;
			}
			break;
		}
		default:
			assert(!"Oh no!");
		}
	}
}

extern volatile struct limine_framebuffer_request fb_request;

// Initializes the graphics backend.
void Init()
{
	struct limine_framebuffer *FrameBuffer =
		fb_request.response->framebuffers[0];

	PixBuff = FrameBuffer->address;
	PixWidth = FrameBuffer->width;
	PixHeight = FrameBuffer->height;
	PixPitch = FrameBuffer->pitch;
	PixBPP = FrameBuffer->bpp;
}

uint64_t ReadTsc()
{
	uint64_t low, high;

	// note: The rdtsc instruction is specified to zero out the top 32 bits of rax and rdx.
	asm volatile("rdtsc" : "=a"(low), "=d"(high));

	// So something like this is fine.
	return high << 32 | low;
}

unsigned RandTscBased()
{
	uint64_t Tsc = ReadTsc();
	return ((uint32_t)Tsc ^ (uint32_t)(Tsc >> 32));
}

int g_randGen = 0x9521af17;
int Rand()
{
	g_randGen += (int)0xe120fc15;
	uint64_t tmp = (uint64_t)g_randGen * 0x4a39b70d;
	uint32_t m1 = (tmp >> 32) ^ tmp;
	tmp = (uint64_t)m1 * 0x12fad5c9;
	uint32_t m2 = (tmp >> 32) ^ tmp;
	return m2 & 0x7FFFFFFF; //make it always positive.
}

typedef int64_t FixedPoint;

#define RAND_MAX 0x7FFFFFFF

#define FIXED_POINT 16 // fixedvalue = realvalue * 2^FIXED_POINT

#define FP_TO_INT(FixedPoint) ((int)((FixedPoint) >> FIXED_POINT))
#define INT_TO_FP(Int) ((FixedPoint)(Int) << FIXED_POINT)

#define MUL_FP_FP(Fp1, Fp2) (((Fp1) * (Fp2)) >> FIXED_POINT)

// Gets a random representing between 0. and 1. in fixed point.
FixedPoint RandFP()
{
	return Rand() % (1 << FIXED_POINT);
}

FixedPoint RandFPSign()
{
	if (Rand() % 2)
		return -RandFP();
	return RandFP();
}

#include "sintab.h"

FixedPoint Sin(int Angle)
{
	return INT_TO_FP(SinTable[Angle % 65536]) / 32768;
}

FixedPoint Cos(int Angle)
{
	return Sin(Angle + 16384);
}

typedef struct _FIREWORK_DATA {
	int m_x, m_y;
	uint32_t m_color;
	FixedPoint m_actX, m_actY; // fixed point
	FixedPoint m_velX, m_velY; // fixed point
	int m_explosionRange;
} FIREWORK_DATA, *PFIREWORK_DATA;

uint32_t GetRandomColor()
{
	return (Rand() + 0x808080) & 0xFFFFFF;
}

void SpawnParticle(PFIREWORK_DATA Data);

#define DELTA_SEC Delay / 1000

void PerformDelay(int ms, void *)
{
	sched_sleep(MS2NS(ms));
}

[[gnu::noreturn]] void T_Particle(void *Parameter, void *)
{
	PFIREWORK_DATA ParentData = Parameter;
	FIREWORK_DATA Data;
	memset(&Data, 0, sizeof Data);

	// Inherit position information
	Data.m_x = ParentData->m_x;
	Data.m_y = ParentData->m_y;
	Data.m_actX = ParentData->m_actX;
	Data.m_actY = ParentData->m_actY;
	int ExplosionRange = ParentData->m_explosionRange;

	kfree(ParentData, sizeof(FIREWORK_DATA));

	int Angle = Rand() % 65536;
	Data.m_velX = MUL_FP_FP(Cos(Angle), RandFPSign()) * ExplosionRange;
	Data.m_velY = MUL_FP_FP(Sin(Angle), RandFPSign()) * ExplosionRange;

	int ExpireIn = 2000 + Rand() % 1000;
	Data.m_color = GetRandomColor();

	int T = 0;
	for (int i = 0; i < ExpireIn;) {
		PlotPixel(Data.m_color, Data.m_x, Data.m_y);

		int Delay = 16 + (T != 0);
		PerformDelay(Delay, NULL);
		i += Delay;
		T++;
		if (T == 3)
			T = 0;

		PlotPixel(BACKGROUND_COLOR, Data.m_x, Data.m_y);

		// Update the particle
		Data.m_actX += Data.m_velX * DELTA_SEC;
		Data.m_actY += Data.m_velY * DELTA_SEC;

		Data.m_x = FP_TO_INT(Data.m_actX);
		Data.m_y = FP_TO_INT(Data.m_actY);

		// Gravity
		Data.m_velY += INT_TO_FP(10) * DELTA_SEC;
	}

	// Done!
	sched_exit_destroy();
}

[[gnu::noreturn]] void T_Explodeable(void *Parameter, void *)
{
	timer_t Timer;
	timer_init(&Timer);
	FIREWORK_DATA Data;
	memset(&Data, 0, sizeof Data);

	int OffsetX = PixWidth * 400 / 1024;

	// This is a fire, so it doesn't have a base.
	Data.m_x = PixWidth / 2;
	Data.m_y = PixHeight - 1;
	Data.m_actX = INT_TO_FP(Data.m_x);
	Data.m_actY = INT_TO_FP(Data.m_y);
	Data.m_velY = -INT_TO_FP(400 + Rand() % 400);
	Data.m_velX = OffsetX * RandFPSign();
	Data.m_color = GetRandomColor();
	Data.m_explosionRange = Rand() % 100 + 100;

	int ExpireIn = 500 + Rand() % 500;
	int T = 0;
	for (int i = 0; i < ExpireIn;) {
		PlotPixel(Data.m_color, Data.m_x, Data.m_y);

		int Delay = 16 + (T != 0);
		PerformDelay(Delay, NULL);
		i += Delay;
		T++;
		if (T == 3)
			T = 0;

		PlotPixel(BACKGROUND_COLOR, Data.m_x, Data.m_y);

		// Update the particle
		Data.m_actX += Data.m_velX * DELTA_SEC;
		Data.m_actY += Data.m_velY * DELTA_SEC;

		Data.m_x = FP_TO_INT(Data.m_actX);
		Data.m_y = FP_TO_INT(Data.m_actY);

		// Gravity
		Data.m_velY += INT_TO_FP(10) * DELTA_SEC;
	}

	// Explode it!
	// This spawns many, many threads! Cause why not, right?!
	int PartCount = Rand() % 30 + 30;

	for (int i = 0; i < PartCount; i++) {
		PFIREWORK_DATA DataClone = kmalloc(sizeof(FIREWORK_DATA));
		if (!DataClone)
			break;

		*DataClone = Data;
		SpawnParticle(DataClone);
	}

	sched_exit_destroy();
}

void SpawnParticle(PFIREWORK_DATA Data)
{
	thread_t *Thrd = sched_alloc_thread();
	sched_init_thread(Thrd, T_Particle, kPriorityMiddle, Data, nullptr);
	Thrd->name = "Particle";
	assert(Thrd);
	sched_resume(Thrd);
	/*
	if (!Thrd)
		MmFreePool(Data);
	else
		ObDereferenceObject(Thrd);
	*/
}

void SpawnExplodeable()
{
	thread_t *Thrd = sched_alloc_thread();
	sched_init_thread(Thrd, T_Explodeable, kPriorityMiddle, nullptr,
			  nullptr);
	Thrd->name = "Explodeable";
	assert(Thrd);
	sched_resume(Thrd);
	/*
	if (Thrd)
		ObDereferenceObject(Thrd);
	*/
}

thread_t *fwt;

void PerformFireworksTest()
{
	fwt = curthread();
	Init();
	g_randGen ^= RandTscBased();

	FillScreen(BACKGROUND_COLOR);

	// The main thread occupies itself with spawning explodeables
	// from time to time, to keep things interesting.
	timer_t Timer;
	timer_init(&Timer);

	curthread()->name = "Fireworks";
	printk("%d\n", curthread()->priority);

	while (true) {
		int SpawnCount = Rand() % 1 + 1;

		for (int i = 0; i < SpawnCount; i++) {
			SpawnExplodeable();
		}

		//PerformDelay(2000 + Rand() % 2000, NULL);
		PerformDelay(100, NULL);
	}
}
