//Jacob Vargo, Guarab KC
//Printf Project

#include "printf.hpp"

int uint64_to_hex(char* dest, uint64_t src);
int int64_to_dec(char* dest, int64_t src);
int uint64_to_double(char* dest, double src);

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
				chars_written = int64_to_dec(dest, value);
				write(1, dest+dest_size-chars_written, chars_written);
			} else if (fmt[i+1] == 'x'){ /* print hex*/
				i++;
				uint64_t value = va_arg(args, uint64_t);
				chars_written = uint64_to_hex(dest, value);
				write(1, dest+dest_size-chars_written, chars_written);
			} else if (fmt[i+1] == 'f'){ /* print double*/
				i++;
				double value = va_arg(args, double);
				chars_written = uint64_to_double(dest, value);
				write(1, dest+dest_size-chars_written, chars_written);
			} else if (fmt[i+1] == 's'){ /* print char* */
				/* optional */
				i++;
				char * value;
				value = va_arg(args, char*);
				while (*value != '\0'){
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

int uint64_to_hex(char* dest, uint64_t src){
	//Assume dest is 100 chars in length
	uint64_t i;
	int dest_length = 100;
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
int int64_to_dec(char* dest, int64_t src){
	//Assume dest is 100 chars in length
	int64_t i;
	int dest_length = 100;
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

int uint64_to_double(char* dest, double src){
	//Assume dest is 100 chars in length
	int i;
	int dest_length = 100;
	int k = dest_length-1;
	bool isNegative = false;
	double x = src;
	if (x < 0){
		isNegative = true;
		x = x * -1;
	}
	int y = x;			// seperate whole number and fraction part
	x = (x-y);			// y has whole number
	int z = x * 1000000;		// z has fraction part and is multiplied by 1000000 to get 6 decimal place
	// do whole and fraction seperate and combine. 
	if (z != 0){
		while(0 <= k && k < dest_length && z != 0){
			i = z%10;
			dest[k] = i + '0';
			z = z / 10;
			k--;
		}
		dest[k]='.';
		k--;
	}
	if (y == 0){
		dest[k] = '0';
		k--;
	} else {
		while (0 <= k && k < dest_length && y != 0){
			i = y%10;
			dest[k] = i + '0';
			y = y / 10;
			k--;
		}
	}
	if (isNegative){
		dest[k] = '-';
		k--;
	}
	return dest_length-1-k;
}

int main(int argc, char **argv){
	long i = 9223372036854775807;
	double d = 123.654321;
	int length;
	printf("Print: 2147483648\n");
	printf("hex: %x\n", i);
	printf("int: %d\n", i);
	printf("float: %f\n", d);
	i = i * -1;
	d = d * -1;
	printf("Print: -2147483648\n");
	printf("hex: %x\n", i);
	printf("int: %d\n", i);
	printf("float: %f\n", d);
	i = 0;
	d = 0;
	printf("Print: 0\n");
	printf("hex: %x\n", i);
	printf("int: %d\n", i);
	printf("float: %f\n", d);
	printf("Print: Hello World\n");
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
