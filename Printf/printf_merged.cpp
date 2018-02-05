//Jacob Vargo, Guarab KC
//Printf Project

#include "printf.hpp"

int uint64_to_hex(char* dest, uint64_t src, int dest_length);
int int64_to_dec(char* dest, int64_t src, int dest_length);
int uint64_to_doubledec(char* dest, double src);

int printf(const char* fmt, ...){
	int bytes_written = 0;
	int chars_written;
	const int dest_size = 100;
	char dest[dest_size];
	va_list args;
	va_start(args, fmt);
	for (int i = 0; fmt[i] != '\0'; i++){
		chars_written = 0;
		if (fmt[i] == '%'){
			if (fmt[i+1] == 'd'){ /* print int */
				i++;
				int64_t value = va_arg(args, int64_t);
				chars_written = int64_to_dec(dest, value, dest_size);
				write(1, dest+dest_size-chars_written, chars_written);
			} else if (fmt[i+1] == 'x'){ /* print hex*/
				i++;
				uint64_t value = va_arg(args, uint64_t);
				chars_written = uint64_to_hex(dest, value, dest_size);
				write(1, dest+dest_size-chars_written, chars_written);
			} else if (fmt[i+1] == 'f'){ /* print double*/
				/* optional: specifier to change precision */
				i++;
				double value = va_arg(args, double);
				chars_written = uint64_to_doubledec(dest, value);
				write(1, dest+dest_size-chars_written, chars_written);
			} else if (fmt[i+1] == 's'){ /* print char* */
				/* optional */
				i++;
				char* value = va_arg(args, char*);
				while(*value != '\0'){
					write(1, value, 1);
					chars_written++;
					value++;
				}
			}
		} else {
			write(1, &(fmt[i]), 1);
			chars_written = 1;
		}
		bytes_written += chars_written;
	}
	va_end(args);
	return bytes_written;
}

int snprintf(char *dest, size_t size, const char *fmt, ...){

	return 0;
}

int uint64_to_hex(char* dest, uint64_t src, int dest_length){
	uint64_t i;
	int k = dest_length-1;

	if (src == 0){
		dest[dest_length-1] = '0';
		return 1;
	}
	while (0 <= k && k < dest_length && src != 0){
		i = src%16;
		if (i < 10) dest[k] = i + '0';
		else dest[k] = (i - 10) + 'A';
		src = src / 16;
		k--;
	}
	return dest_length-1-k;
}
int int64_to_dec(char* dest, int64_t src, int dest_length){
	int i;
	int k = dest_length-1;
	bool isNegative = false;

	if (src == 0){
		dest[dest_length-1] = '0';
		return 1;
	}
	if (src < 0){
		isNegative = true;
		src = src * -1;
	}
	while (0 <= k && k < dest_length && src != 0){
		i = src%10;
		dest[k] = i + '0';
		src = src / 10;
		k--;
	}
	if (isNegative){
		dest[k] = '-';
		k--;
	}
	return dest_length-1-k;
}

int uint64_to_doubledec(char* dest, double src){
	int dest_length = 100;
	double x;
	x = src;
	int y = x;	// seperate whole number and fraction part
	double z = (x-y);	// y has whole number
	if(z<0){
		z = -z;
	}
	z=z*1000000 + .5;		// z has fraction part and is multiplied by 1000000 to get 6 decimal place
	int w = z;
	int k = 99;		// do whole and fraction seperate and combine. 	
	k-= int64_to_dec(dest, w, dest_length);
	dest[k]='.';
	k--;
	k-= int64_to_dec(dest, y, k+1);
	return 99-k;
}


int main(int argc, char **argv){
	long i = 9223372036854775807;
	int length;
	printf("Print: 9223372036854775807\n");
	printf("hex: %x\n", i);
	printf("int: %d\n", i);
	i = i * -1;
	printf("Print -9223372036854775807\n");
	printf("hex: %x\n", i);
	printf("int: %d\n", i);
	i = 0;
	printf("Print: 0\n");
	printf("hex: %x\n", i);
	printf("int: %d\n", i);
	double d = 12.654321;
	printf("Print: 12.654321\n");
	printf("double: %f\n", d);
	d = d * -1;
	printf("negative double: %f\n", d);
	d = 0;
	printf("zero double: %f\n", d);
	printf("string: Hello World\n");
	char str[] = "Hello World";
	printf("%s\n", str);
	length = printf("1234567890\n");
	printf("%d\n", length);
	i = 1234567890;
	length = printf("%d\n", i);
	printf("%d\n", length);
	length = printf("%x\n", i);
	printf("%d\n", length);
	d = 10.5;
	length = printf("%f\n", d);
	printf("%d\n", length);
	return 0;
}

