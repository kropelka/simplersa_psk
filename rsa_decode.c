#include <gmp.h>
#include <stdio.h>
#include <newt.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define ASCII 256


/*! znajdowanie rozmiaru pliku
 *
 * Uwaga: funkcja przesuwa znacznik pozycji w pliku na początek!
 */
long int file_size(FILE* file) {
  fseek(file, 0, SEEK_END);
  long int x = ftell(file);
  fseek(file, 0, SEEK_SET);
  return x;
}

/*! kodowanie pliku binarnego zgodnie z algorytmem RSA.
 *
 * Funkcja ta pobiera ze strumienia input_file bloki znaków o wielkości
 * określonej przez block_size, koduje je jako liczbę całkowitą w której
 * kolejne znaki są cyframi w jej rozwinięciu przy podstawie 256.
 * \param input_file strumień wejściowy
 * \param output_file strumień wyjściowy
 * \param n część klucza (n,e)
 * \param e publiczna część klucza
 * \param block_size wielkość bloku wejściowego w bajtach
 * \param out_block_size wielkość bloku wyjściowego w bajtach
 * \param verbose czy pokazywać progress bar
 * \todo zamienić ręczne rzeźbienie z konwersją blokową na mpz_import
 */
void rsa_encode_file(FILE* input_file, FILE* output_file, mpz_t* n, mpz_t* e, int block_size, int out_block_size, int verbose) {
  clock_t encode_timer;
  float curr_time = 0;
  long int input_file_size = file_size(input_file);
  long int size_progress = 0;
  mpz_t message, result, power, pot, temp;
  mpz_init(message);
  mpz_init(result);
  mpz_init(power);
  mpz_init_set_ui(pot,1);
  mpz_init(temp);
  int readed_blocks,j;
  int padded_bytes = 0;
  unsigned char block[block_size];
  unsigned char out_block[out_block_size];

  newtComponent progress_bar = newtScale(1, 1, 58, input_file_size);
  char* size_label_text = malloc(58);
  char* time_label_text = malloc(58);
  char* velocity_label_text = malloc(58);
  newtComponent size_label = newtLabel(1, 3, "");
  newtComponent time_label = newtLabel(1, 4, "");
  newtComponent velocity_label = newtLabel(1, 5, "");
  newtComponent form = newtForm(NULL, NULL, 0);
  newtFormAddComponents(form, progress_bar, size_label, time_label, velocity_label, NULL);
  if(verbose) {
    newtCenteredWindow(60, 6, "Trwa kodowanie...");
    newtDrawForm(form);
  };
  
  encode_timer = clock();
  
  while(block_size == (readed_blocks = fread(&block[0], 1, block_size, input_file))) {
      curr_time = ((float)(clock() - encode_timer))/CLOCKS_PER_SEC;
    if(verbose) {
      size_progress += block_size;
      snprintf(size_label_text, 58, "Zakodowano %ld / %ld bajtów.", size_progress, input_file_size);
      newtLabelSetText(size_label, size_label_text);
      snprintf(time_label_text, 58, "Upłynęło %f sekund.", curr_time);
      newtLabelSetText(time_label, time_label_text);
      snprintf(velocity_label_text, 58, "Średnia prędkość: %f kB/s", size_progress/(1000*curr_time) );
      newtLabelSetText(velocity_label, velocity_label_text);
      newtScaleSet(progress_bar, size_progress);
      newtRefresh();    
    };
    
    mpz_set_ui(message, 0);
    if(readed_blocks < block_size) {
      padded_bytes = block_size - readed_blocks;
      for(j=readed_blocks; j < block_size; ++j)
        block[j] = (unsigned char) padded_bytes;
    };
    mpz_set_ui(pot,1);
    for(j=0; j<block_size; ++j) {
      mpz_mul_ui(temp, pot, block[j]);
      mpz_add(message, message, temp);
      mpz_mul_ui(pot, pot, ASCII);
    };
    mpz_powm(result, message, *e, *n);
    for(j=0; j<out_block_size; ++j) {
      out_block[j] = (unsigned char) mpz_tdiv_q_ui(result, result, ASCII);
    };
    fwrite(out_block, 1, out_block_size, output_file);
  };

  if(padded_bytes==0) {
    for(j=0; j < block_size; ++j)
      out_block[j] = (unsigned char) block_size;
    fwrite(out_block, 1, out_block_size, output_file);
  };
  char* summary_msg = malloc(strlen("Kodowanie zakończyło się sukcesem po ") + log10((long)(curr_time + 0.5)) + strlen("sekundach.") + 5 );
  sprintf(summary_msg, "Kodowanie zakończyło się sukcesem po %ld sekundach.", (long)(curr_time + 0.5));
  newtWinMessage("","OK",summary_msg);
  free(summary_msg);
  mpz_clear(message);
  mpz_clear(result);
  mpz_clear(pot);
  mpz_clear(temp);
  mpz_clear(power);
  free(size_label_text);
  free(time_label_text);
  free(velocity_label_text);
  newtFormDestroy(form);
  if(verbose) {
    newtPopWindow();

  };
}

/*! Dekodowanie pliku binarnego zgodnie z algorytmem RSA.
 *
 * Funkcja ta pobiera z zakodowanego strumienia input_file bloki znaków o wielkości
 * określonej przez block_size, koduje je jako liczbę całkowitą w której
 * kolejne znaki są cyframi w jej rozwinięciu przy podstawie 256. Ze względu na 
 * sposób działania funkcji bibliotecznej fread() musimy wprost sprawdzać, w którym
 * miejscu pliku jesteśmy.
 * \param input_file strumień wejściowy
 * \param output_file strumień wyjściowy
 * \param n część klucza (n,d)
 * \param d prywatna część klucza
 * \param block_size wielkość bloku wejściowego w bajtach
 * \param out_block_size wielkość bloku wyjściowego w bajtach
 * \param verbose czy pokazywać progress bar
 * \todo zamienić ręczne rzeźbienie z konwersją blokową na mpz_import
 */
void rsa_decode_file(FILE* input_file, FILE* output_file, mpz_t* n, mpz_t* d, int block_size, int out_block_size, int verbose) {
  long int input_file_size = file_size(input_file);
  clock_t encode_timer;
  float curr_time = 0;
  long int size_progress = 0;
  mpz_t message, result, remainder, power, pot, temp;
  mpz_init(message);
  mpz_init(result);
  mpz_init(remainder);
  mpz_init(power);
  mpz_init(pot);
  mpz_init(temp);
  unsigned char block[block_size];
  unsigned char out_block[out_block_size];
  int j, readed_bytes;
  int padded_bytes = 0;
  newtComponent progress_bar = newtScale(1, 1, 58, input_file_size);
  char* size_label_text = malloc(58);
  char* time_label_text = malloc(58);
  char* velocity_label_text = malloc(58);
  newtComponent size_label = newtLabel(1, 3, "");
  newtComponent time_label = newtLabel(1, 4, "");
  newtComponent velocity_label = newtLabel(1, 5, "");
  newtComponent form = newtForm(NULL, NULL, 0);
  newtFormAddComponents(form, progress_bar, size_label, time_label, velocity_label, NULL);
  if(verbose) {
    newtCenteredWindow(60, 6, "Trwa dekodowanie...");
    newtDrawForm(form);
  };
  
  encode_timer = clock();
  
  while((readed_bytes = fread(&block[0], 1, block_size, input_file))) {
    curr_time = ((float)(clock() - encode_timer))/CLOCKS_PER_SEC;
    if(verbose) {
      size_progress += block_size;
      snprintf(size_label_text, 58, "Zdekodowano %ld / %ld bajtów.", size_progress, input_file_size);
      newtLabelSetText(size_label, size_label_text);
      snprintf(time_label_text, 58, "Upłynęło %f sekund.", curr_time);
      newtLabelSetText(time_label, time_label_text);
      snprintf(velocity_label_text, 58, "Średnia prędkość: %f kB/s", size_progress/(1000*curr_time) );
      newtLabelSetText(velocity_label, velocity_label_text);
      newtScaleSet(progress_bar, size_progress);
      newtRefresh();    
    };
    mpz_set_ui(message, 0);
    /* w przypadku, gdy jesteśmy na końcu pliku, pobierz miejsce obcięcia ciągu*/
    if(ftell(input_file) == input_file_size) {
      padded_bytes = block[readed_bytes-1];
    };
    /* zapisz blok jako dużą liczbę */
    mpz_set_ui(pot, 1);
    for(j=0; j<block_size; ++j) {
      mpz_mul_ui(temp, pot, block[j]);
      mpz_add(message, message, temp);
      mpz_mul_ui(pot, pot, ASCII);
    };
    /* właściwe RSA */
    mpz_powm(result, message, *d, *n);
    /* zmień zakodowaną liczbę z powrotem w ciąg bajtów */
    for(j=0; j<out_block_size; ++j) {
      out_block[j] = (unsigned char) mpz_tdiv_q_ui(result, result, ASCII);
    };
    /* tutaj bierzemy pod uwagę padding */
    fwrite(&out_block[0], 1, out_block_size - padded_bytes, output_file);
  };
  char* summary_msg = malloc(strlen("Dekodowanie zakończyło się sukcesem po ") + log10((long)(curr_time + 0.5)) + strlen("sekundach.") + 2 );
  sprintf(summary_msg, "Dekodowanie zakończyło się sukcesem po %ld sekundach.", (long)(curr_time + 0.5));
  newtWinMessage("","OK",summary_msg);
  free(summary_msg);
  
  mpz_clear(message);
  mpz_clear(result);
  mpz_clear(pot);
  mpz_clear(temp);
  mpz_clear(power);
  newtFormDestroy(form);
  free(size_label_text);
  free(time_label_text);
  free(velocity_label_text);
  if(verbose) {
    newtPopWindow();
  };
}
