/*
	Reference: http://www.di.unipi.it/~nids/docs/longjump_try_trow_catch.html
*/
#include <stdio.h>
#include <string.h>
#define REG_SIZE 8

struct js_jmp_buf {
	void *ip; //0
	void *bp; //8
	void *sp; //16
	unsigned char stack[REG_SIZE * 2]; //saved frame ptr and return address
};

int __attribute__((noinline))
js_setjmp(struct js_jmp_buf *buf) {
	void *a = NULL;
	asm volatile("movq %%rbp, %0" : "=r" (a));
	memcpy(buf->stack, a, REG_SIZE * 2); //copy saved frame ptr and return address
	buf->bp = a;
	asm volatile("movq %%rsp, %0" : "=r" (a));
	buf->sp = a;
	asm volatile("movq $1f, (%0); movl $0x0, %%eax; 1:"
		: "=g" (buf));
}

void __attribute__((noinline)) 
js_longjmp(struct js_jmp_buf *buf, int error_code) {
	memcpy(buf->bp, buf->stack, REG_SIZE * 2);//recover stack
	//recover the enviroment, set eax to error_code, jmp to label 1 in js_setjmp
	asm volatile ("movq %0, %%rbx; \
								movq %1, %%rax; \
								movq 8(%%rbx), %%rbp; \
								movq 16(%%rbx), %%rsp; \
								jmp *(%%rbx)"
		::"g"(buf), "g"(error_code));
}
#define TRY do { int __try_result = 0; \
	do { struct js_jmp_buf __jmp_buf, *__pjmp_buf = &__jmp_buf; \
	switch (__try_result = js_setjmp(&__jmp_buf)) { case 0: 
#define GET_ERR_NO() __try_result
#define _end_prev_block __try_result = 0; break
#define CATCH(ERROR_NO) _end_prev_block; case ERROR_NO:
#define THROW(ERROR_NO) js_longjmp(__pjmp_buf, ERROR_NO)
#define CATCH_ALL _end_prev_block; default: 
#define END_TRY  _end_prev_block; } } while (0);} while(0)
#define END_NEST_TRY  _end_prev_block; } } while (0);\
	if(__try_result) THROW(__try_result); } while(0)
#define CAN_THROW struct js_jmp_buf *__pjmp_buf
#define THROW_ENV __pjmp_buf

#define Exception1 1
#define Exception2 2
#define Exception_DivZero 3
double div(int a, int b, CAN_THROW);
double sub(int a, int b, CAN_THROW) {
	TRY{
		printf("in sub(), call div(0,0)\n");
		div(0, 0, THROW_ENV);
		printf("should not print\n");
	} CATCH(Exception1) {
		printf("sub only catches exception 1\n");
	} END_NEST_TRY;
}
double div(int a, int b, CAN_THROW) {
	if (a == 0 && b == 0) {
		THROW(Exception_DivZero);
	}
	return a / b;
}
int main(int argc, char *argv[]) {

	printf("sizeof(int *) = %ld\n", sizeof(int *));
	TRY{
		printf("hhh\n");
		sub(0, 0, THROW_ENV);
		printf("should not print\n");
	}
	CATCH(Exception1) {
		printf("catched exception 1!\n");
	}
	CATCH(Exception2) {
		printf("catched exception 2!\n");
		THROW(Exception1);
	}
	CATCH_ALL {
		printf("main() catched exception %d!\n", GET_ERR_NO());
	}
	END_TRY;
	return 0;
}