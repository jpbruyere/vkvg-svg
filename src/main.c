#include <stdio.h>
#include <errno.h>
#include <stdint.h>

int main(){
	const char* filename = "/mnt/data/images/svg/tiger.svg";
	FILE* f = fopen(filename, "r");
	if (f == NULL){
		printf("error: %d\n", errno);
		return -1;
	}
	char elt[128];
	char att[1024];
	char value[5096];

	while (!feof(f)) {
		//fscanf(f, "[ |\n|\r|\t]");//skip whitespaces
		int res = fscanf(f, " <%[^> ]", elt);
		if (res < 0)
			break;
		if (!res) {
			printf("element name parsing error\n");
			return -1;
		}
		if (elt[0] == '/') {
			printf("\n<%s ", elt);
			res = fscanf(f, "  %c", elt);
			if (!res || elt[0] != '>') {
				printf("parsing error, expecting '>'\n");
				return -1;
			}
			printf(">\n");
			continue;;
		}
		printf("<%s ", elt);
		fflush(stdout);

		res = fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", att, value);
		while (res == 2) {
			printf("%s='%s' ", att, value);
			fflush(stdout);
			res = fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", att, value);
		}
		if (res == 0)
			printf(">\n");
		else if (res == 1)
			printf("%s>\n", att);
		/*if (!res || (att[0] != '>' && !(att[0]=='/' && att[1]=='>'))) {
			printf("parsing error, expecting '>'\n");
			return -1;
		}*/
		if (getc(f) != '>') {
			printf("parsing error, expecting '>'\n");
			return -1;
		}
		fflush(stdout);
	}



	fclose(f);
}
