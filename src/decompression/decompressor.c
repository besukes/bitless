/*  This module is about decompressing a file compressed using the code provided in this repository , 
it may (and most likely WILL) NOT WORK on a file compressed using other constructors.
    This module expects the file to have a header in the following format :
        ------------------------------
            LAST RELEVANT BITS (1 BYTE)
            Canonic TABLE
            end
        ------------------------------
    If the file does not contain this format , it will throw an error and will not execute.
    The decompression itself cannot be done as of now but i'll eventually add it to the main function as a
text argument that can be sent before the paths.
*/
#include "src/includes/types.h"
#include "src/compression/compressor.h"
#include "src/initialization/init_clean.h"
#include "src/writing/write_out.h"
#include <stdio.h>
#include <sys/stat.h>

#define NO_ERRORS 0
#define INVALID_PATH 1
#define INVALID_HEADER_FORMAT 2
#define NO_PATH 3
#define ALREADY_EXISTING_FOLDER 4
#define BUFFER_SIZE 20480 //20Kib







/*Error handler for Decompression function*/
static int dc_error_handler(int flags , int indx){
    if(flags == INVALID_PATH){
        printf("[ERROR] Decompression was tried on a invalid path at index %d\n",indx);
        return (INVALID_PATH);
    }
    else if(flags == INVALID_HEADER_FORMAT){
        printf("[ERROR] The file provided by the path at position %d doesn't include a decompression header ,"
                "or includes one in an invalid format\n",indx);
        return (INVALID_HEADER_FORMAT);
    }
    else if(flags == NO_PATH){
        printf("[ERROR] No paths were included at run time\n");
        return (NO_PATH);
    }
    else if(flags == ALREADY_EXISTING_FOLDER){
        printf("[ERROR] Error while making the decompressed file\n");
        return (ALREADY_EXISTING_FOLDER);
    }
    return 0;
}





/*Reads the frequency table of a compressed file header and stores the information directly onto f_info*/
static int readHeaderBuffer(char * buffer , int current_indx , int size_buffer , int parser[MAX_SYMBOLS] , int * nmbr_pairs){
    int i=current_indx;
    while(i<size_buffer && *nmbr_pairs > 0){
        unsigned char symb = (unsigned char)buffer[i];
        parser[symb] = buffer[i+1];
        if(buffer[i+1] == '\0') i++;
        else i+=2;
        (*nmbr_pairs)--;
    }
    return i;
}




/*Parses file header to store the information on each symbol that exists and keeps its frequency to calculate
the probability to then apply the Shannon-Fano's algorithm to decompress the file*/
static int parseFileHeader(FILE* file,CompressInfo* f_info,int * lrb,char * buffer , int * buffer_cur_indx , int * h_size){
    //Reads the last relevant bits in the last relevant byte , stored at the start of the header
    size_t read_bytes = fread(buffer,1,BUFFER_SIZE,file);
    *lrb = (int)buffer[0];

    int number_symbols = (int)buffer[1] , keep_searching = number_symbols;
    *buffer_cur_indx += 3;
    *h_size = 1;


    //Reads the Frequency table of each symbol to reconstruct the probabilities table
    int parser[MAX_SYMBOLS] = {0};
    while(keep_searching){
        *buffer_cur_indx = readHeaderBuffer(buffer,*buffer_cur_indx,read_bytes,parser,&keep_searching);
        *h_size += *buffer_cur_indx;
        if(keep_searching){
            read_bytes = fread(buffer,1,BUFFER_SIZE,file);
            *buffer_cur_indx = 0;
        }
    }
    *buffer_cur_indx+=1;

    f_info->n_symbols = number_symbols;
    f_info->symbols = malloc(sizeof(char)*number_symbols);
    f_info->newcode_arr = malloc(sizeof(NewSymbols)*number_symbols);

    for(int i=0,j=0;i<256;i++){
        if(parser[i]){
            f_info->symbols[j] = i;
            f_info->newcode_arr[j].length = parser[i];
            f_info->newcode_arr[j].new_code = 0;
            j++;
        }
    }

    //Sorts both the arrays in an crescent order , by code length
    sortByCodeLength(f_info->symbols,f_info->newcode_arr,number_symbols);
    
    //If the code has a length superior to the index before him , then adding one wont result on a bigger length most likely
    for(int n=1;n<number_symbols;n++){
        if(f_info->newcode_arr[n-1].length < f_info->newcode_arr[n].length){
            int shift = f_info->newcode_arr[n].length - f_info->newcode_arr[n-1].length;
            f_info->newcode_arr[n].new_code = (f_info->newcode_arr[n-1].new_code + 1) << shift;
        }
        else f_info->newcode_arr[n].new_code = f_info->newcode_arr[n-1].new_code + 1;
    }
    return (NO_ERRORS);
}




/*Decompresses the file back to normal given the information after the execution of Shannon-Fano's algorithm*/
static void decompress(char * buffer , int buffer_cur_indx , int header_size , FILE* ext_file ,CompressInfo* f_info , int lrb){
    FILE * cmp_file = f_info->file;
    unsigned long long output_code = 0;
    int code_size = 0;

    //get the file information and size
    int descritor = fileno(cmp_file);
    struct stat st;fstat(descritor,&st);
    long file_size = st.st_size;
    long remaining_file_size = file_size - header_size , file_read_size = 0;

    while(file_read_size < remaining_file_size){
        if(buffer_cur_indx >= BUFFER_SIZE){
            fread(buffer,1,BUFFER_SIZE,cmp_file);
            buffer_cur_indx = 0;
        }
        for(;buffer_cur_indx < BUFFER_SIZE && file_read_size < remaining_file_size;buffer_cur_indx++,file_read_size++){
            unsigned char byte = buffer[buffer_cur_indx];
            int read_bits = ((remaining_file_size - file_read_size == 1) && lrb) ? lrb : 8; //Verifies if we are at the last byte
            while(read_bits){
                unsigned char mrb = (byte >>7) & 1; //Char has 8 bits , we get the most relevant bit
                byte<<=1;
                output_code = (output_code << 1) | mrb;
                code_size++;
                read_bits --;
                for(int j=0;j<f_info->n_symbols;j++){
                    int length = f_info->newcode_arr[j].length;
                    if(length == code_size){
                        if(output_code == f_info->newcode_arr[j].new_code){
                            fprintf(ext_file,"%c",f_info->symbols[j]);
                            code_size = 0;
                            output_code = 0;
                        }
                    }
                }
            }
        }
    }
    //Still need to take care of the last file byte and use lrb for the last relevant bits in the last byte
}


/*Decompression function that grabs a compressed file and decompresses it back to normal */
static int decompression_handler(char * path , CompressInfo* f_info){

    FILE * compressed_file = fopen(path,"r");
    if(compressed_file == NULL) return (INVALID_PATH);

    int lrb = 0; //Last relevant bits in the file on the last byte
    char buffer[BUFFER_SIZE];
    int buffer_cur_indx = 0 , header_size = 0;
    int parsing_error = parseFileHeader(compressed_file,f_info,&lrb,buffer,&buffer_cur_indx,&header_size);

    if(parsing_error) return (INVALID_HEADER_FORMAT);
    
    f_info->file = compressed_file;
    f_info->file_path = path;

    int size = strlent(f_info->file_path);
    char new_path[size + 1];
    append_before_format(f_info->file_path,new_path,".txt");

    FILE * new_file = fopen(new_path,"r");
    if(new_file != NULL){
        fclose(new_file);
        printf("There's already a file named %s , therefore the program won't overwrite\n",new_path);
        return (ALREADY_EXISTING_FOLDER);
    }

    new_file = fopen(new_path,"w");
    decompress(buffer,buffer_cur_indx,header_size,new_file,f_info,lrb);
    fclose(new_file);

    return (NO_ERRORS);
}


/* Main function that decompresses every path given to it at run time.*/
int main(int argc , char ** argv){
    if(argc < 2) return dc_error_handler(NO_PATH,0);
    CompressInfo file_info = init_compress_info();
    int error = 0;
    for(int i=1;i<argc;i++){
        resetCompressionInfo(&file_info);

        int error_arrived = decompression_handler(argv[i],&file_info);
        if(error_arrived) error = dc_error_handler(error_arrived,i);

        if(error) break;
    }
    free_memory(&file_info);
    return 0;
}