/*! \file simplersa.c
 * \brief SimpleRSA, proste narzędzie do szyfrowania RSA
 * \details Projekt zaliczeniowy z przedmiotu Podstawy Programowania II.
 * \author Karol Piotrowski
 * \author Szczepan Lis
 * \version 0.1
 * \date 2014
 */

#include <stdio.h>
#include <gmp.h>
#include <unistd.h>
#include <stdlib.h>
#include "rsa_keygen.h"
#include "rsa_decode.h"
#include "main_menu.h"
#include <locale.h>
#include <limits.h>
#include <errno.h>
#include <newt.h>
#include <math.h>
#include <string.h>
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

char* nazwy_menu[] = {
  "Generuj nowy klucz RSA",
  "Wczytaj klucz z pliku",
  "Szyfruj plik",
  "Deszyfruj plik",
  "Wyjście"
};

int key_loaded = 0;
int public_key_loaded =0;
int private_key_loaded = 0;
mpz_t n, d, e;
char* SPLASH[] = {
  " ____  _                 _      ____  ____    _",
  "/ ___|(_)_ __ ___  _ __ | | ___|  _ \\/ ___|  / \\",
  "\\___ \\| | '_ ` _ \\| '_ \\| |/ _ \\ |_) \\___ \\ / _ \\",
  " ___) | | | | | | | |_) | |  __/  _ < ___) / ___ \\",
  "|____/|_|_| |_| |_| .__/|_|\\___|_| \\_\\____/_/   \\_\\",
  "                  |_|"
};

int file_exists(char* filename) {
  FILE* file;
  if((file = fopen(filename, "r")) == NULL) {
    return 0;
  } else {
    fclose(file);
    return 1;
  };
}

/*! wyświetlanie informacji o programie na root window
 *
 * Ze względu na doskonałą obsługę stringów w języku C oraz fakt, że
 * toolkit NEWT nie posiada żadnego wyświetlania na ekranie przyjmującego
 * string formatujący, wygląda to dosyć podejrzanie, ale z pewnością
 * działa.
 */
void show_main_screen() {
  char helpline_1[] = "Załadowano klucz publiczny, długość: n ";
  char helpline_2[] = "Załadowano klucz prywatny, długość: n ";
  char helpline_3[] = "bit, d ";
  char helpline_4[] = "bit, e ";
  char helpline_5[] = " bit";
  int y, x = 0;
  newtCls();
  int j;
  for(j=0; j < 6; ++j)
    newtDrawRootText(1, 1+j, SPLASH[j]);
  newtDrawRootText(1, 7, "wersja 0.1");
  newtDrawRootText(1, 8, "(c) 2014 by Karol Piotrowski & Szczepan Lis");
  char* helpline;
  if(public_key_loaded) {
    x = mpz_sizeinbase(e, 2);
    y = mpz_sizeinbase(n, 2);
    helpline = malloc(strlen(helpline_1) + strlen(helpline_3) + strlen(helpline_5) + log10(x) + log10(y) + 3);
    sprintf(helpline, "%s%d%s%d%s",helpline_1, y, helpline_3, x,helpline_5 );
    newtPushHelpLine(helpline);
    free(helpline);
  } else if(private_key_loaded) {
    x = mpz_sizeinbase(d, 2);
    y = mpz_sizeinbase(n, 2);
    helpline = malloc(strlen(helpline_2) + strlen(helpline_4) + strlen(helpline_5) + log10(x) + log10(y) + 3);
    sprintf(helpline, "%s%d%s%d%s",helpline_2, y, helpline_4, x,helpline_5 );
    newtPushHelpLine(helpline);
    free(helpline);
  };
  if(public_key_loaded && private_key_loaded)
    newtPushHelpLine("Wygenerowano nowe klucze RSA.");
  newtRefresh();
}

void handle_key_load_error_code(int error_code, int key_type) {
    switch(error_code) {
      case 0:
        key_loaded = 1;
        newtWinMessage("","OK", "Prawidłowo wczytano klucz.");
        if(key_type==0) {
          private_key_loaded = 1;
          public_key_loaded = 0;
        } else {
          public_key_loaded = 1;
          private_key_loaded = 0;
        };
        break;
      case 1:
        newtWinMessage("", "OK", "To nie jest prawidłowy plik z kluczem RSA.\n");
        break;
      case 2:
      newtWinMessage("", "OK", "Plik nie istnieje bądź nie ma uprawnień do jego odczytu. \n");
        break;
  };
}


int handle_key_load() {
  int key_type = 0; 
  int error_code;
  newtComponent load_form = newtForm(NULL, NULL, 0);
  newtComponent browse_button = newtButton(2, 3, "Przeglądaj...");
  newtComponent ok_button = newtButton(25, 3, "OK");
  newtComponent cancel_button = newtButton(35, 3, "Anuluj");
  char* file_name;
  newtComponent filename_label = newtLabel(1, 1, "Podaj nazwę pliku z kluczem");
  newtComponent filename_field = newtEntry(30, 1, "przyklad", 30, &file_name, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
  newtCenteredWindow(60, 7, "Wczytywanie klucza z pliku");
  newtFormAddComponents(load_form, filename_label, filename_field, browse_button, ok_button, cancel_button, NULL);
  newtComponent form_result = newtRunForm(load_form);
  if(form_result==ok_button || form_result==filename_field) {
    error_code = rsa_plaintext_to_key(file_name, &n, &d, &e, &key_type);    
    handle_key_load_error_code(error_code, key_type);
  } else if(form_result==browse_button) {
    filename_dialog("Wybierz plik z kluczem", &file_name, ".");
    error_code = rsa_plaintext_to_key(file_name, &n, &d, &e, &key_type);
    handle_key_load_error_code(error_code, key_type);
  };
  newtFormDestroy(load_form);
  newtPopWindow();
  return 0;
}


int handle_key_generate() {
  char * key_size_str, *priv_str, *publ_str, result;
  newtComponent keygen_form = newtForm(NULL, NULL, 0);
  newtComponent keygen_label = newtLabel(9, 1, "Oczekiwany rozmiar klucza: ");
  newtComponent keygen_len_field = newtEntry(35, 1, "1024", 6, &key_size_str, NEWT_FLAG_SCROLL);
  newtComponent keygen_label2 = newtLabel(1, 2, "Plik do zapisu klucza prywatnego:");
  newtComponent keygen_priv_file = newtEntry(35, 2, "priv.key", 20, &priv_str, NEWT_FLAG_SCROLL);
  newtComponent keygen_label3 = newtLabel(1, 3, "Plik do zapisu klucza publicznego:");
  newtComponent keygen_publ_file = newtEntry(35, 3, "publ.key", 20, &publ_str, NEWT_FLAG_SCROLL);
  newtComponent keygen_long_checkbox = newtCheckbox(1, 5, "Krótki wykładnik klucza publicznego", '*', NULL, &result);
  newtComponent ok_button = newtButton(2, 7,  "OK");
  newtComponent cancel_button = newtButton(12, 7, "Anuluj");
  newtCenteredWindow(61, 11, "Generowanie klucza RSA");
  newtFormAddComponents(keygen_form, keygen_label, keygen_len_field, keygen_label2, 
     keygen_priv_file, keygen_label3, keygen_publ_file, keygen_long_checkbox, ok_button, cancel_button, NULL);
  newtComponent form_result = newtRunForm(keygen_form);
  if(form_result==ok_button) {
    newtRefresh();
    int key_size = atoi(key_size_str);
    if(!(key_size>0)) {
      newtWinMessage("Błąd", "OK", "Podano nieprawidłowy rozmiar klucza RSA.");
      return 0;
    };
    if(file_exists(priv_str)) {
      newtWinMessage("Błąd", "OK", "Plik do zapisu klucza prywatnego już istnieje bądź nie ma uprawnień do zapisu we wskazanej lokalizacji.");
      return 0;
    };
    if(file_exists(publ_str)) {
      newtWinMessage("Błąd", "OK", "Plik do zapisu klucza publicznego już istnieje bądź nie ma uprawnień do zapisu we wskazanej lokalizacji.");
      return 0;
    };
    rsa_keygen(key_size, &n, &d, &e, (result=='*') ? 0 : 1 );
    public_key_loaded = 1;
    private_key_loaded = 1;
    rsa_key_to_plaintext(priv_str, &n, &d, 0);
    rsa_key_to_plaintext(publ_str, &n, &e, 1);
  };
  newtFormDestroy(keygen_form);
  newtPopWindow();
  return 0;
}

/*! Główna logika aplikacji w zakresie szyfrowania
 * 
 * \param deciphering 0 dla szyfrowania, 1 dla deszyfrowania
 * \todo ulepszyć trochę file-dialog i umieścić go tutaj
 */
int handle_cipher(int deciphering) {
  FILE* inputfile, *outputfile;
  char selected_verbose;
  int right_key_loaded = deciphering ? private_key_loaded : public_key_loaded;
  if(!right_key_loaded) {
    newtWinMessage("Błąd", "OK", "Brak wczytanego klucza.");
    return 0;
  };
  char* input_filename, *output_filename;
  newtComponent decipher_form = newtForm(NULL, NULL, 0);
  newtComponent input_label = newtLabel(1,1, "Plik wejściowy: ");
  newtComponent output_label = newtLabel(1,2, "Plik wyjściowy: ");
  newtComponent input_field = newtEntry(18, 1, '\0', 25, &input_filename, NEWT_FLAG_SCROLL);
  newtComponent output_field = newtEntry(18, 2, '\0', 25, &output_filename, NEWT_FLAG_SCROLL);
  newtComponent verbose_checkbox = newtCheckbox(1, 3, "Wyświetlaj pasek postępu", '*', NULL, &selected_verbose);
  newtComponent ok_button = newtButton(1, 5, "OK");
  newtComponent cancel_button = newtButton(10, 5, "Anuluj");
  newtFormAddComponents(decipher_form, input_label, output_label, input_field, output_field, verbose_checkbox, ok_button, cancel_button, NULL);
  if(!deciphering) {
    newtCenteredWindow(48, 10, "Szyfrowanie");
  } else
    newtCenteredWindow(48, 10, "Deszyfrowanie");
  newtComponent form_result = newtRunForm(decipher_form);
  if(form_result == ok_button)
  {
    if(!file_exists(input_filename)) {
      newtWinMessage("Błąd", "OK", "Wskazany plik wejściowy nie istnieje.");
      return 0;
    };
    if(file_exists(output_filename)) {
      newtWinMessage("Błąd", "OK", "Wskazany plik wyjściowy już istnieje.");
      return 0;
    };
    inputfile = fopen(input_filename, "r");
    outputfile = fopen(output_filename, "w");
    int key_size = mpz_sizeinbase(n, 2);
    int input_chunk = floor((key_size-1)/8) < 256 ? floor((key_size-1)/8) < 256 : 256;
    int output_chunk = 1 + floor(key_size/8);
    int is_verbose = (selected_verbose == '*') ? 1 : 0;
    if(!deciphering) { 
      rsa_encode_file(inputfile, outputfile, &n, &e, input_chunk, output_chunk, is_verbose);
    } else
      rsa_decode_file(inputfile, outputfile, &n, &d, output_chunk, input_chunk, is_verbose);
    fclose(inputfile);
    fclose(outputfile);
    
  };
  newtFormDestroy(decipher_form);
  newtPopWindow();
  return 0;
}


int main(int argc, char* argv[]) {
  mpz_init(n);
  mpz_init(d);
  mpz_init(e);
  int sel_menu_pos = 0;
  int quitting = 0;
  newtInit();
  newtCls();
  while(!quitting) {
    show_main_screen();
    sel_menu_pos = single_choice_menu(ARRAY_SIZE(nazwy_menu), nazwy_menu, "Menu główne");
    switch(sel_menu_pos) {
      case 0:
        handle_key_generate();
        break;
      case 1:
        handle_key_load();
        break;
      case 2:
        handle_cipher(0);
        break;
      case 3:
        handle_cipher(1);
        break;
      case 4:
        quitting = 1;
        break;
      };
   show_main_screen();
  };
  mpz_clear(n);
  mpz_clear(d);
  mpz_clear(e);
  newtFinished();
  return 0;
}
