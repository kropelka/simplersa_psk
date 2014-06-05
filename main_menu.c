#include <stdio.h>
#include <newt.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

/*! proste okno do wyboru pliku do wczytania
 *
 * Kod jest dość zagmatwany ze względu na fakt, że wykorzystujemy wskaźniki z tablicy
 * namelist również jako klucze do listy, którą wyświetlamy za pomocą Listboxa z NEWTa.
 * W wersji na Linuksa wypada zmienić sizedir na sizedir64, żeby nie pojawiały się problemy
 * z plikami 2GB, akurat ten fragment kodu był pisany i testowany na Mac OS X, który w 
 * bibliotece standardowej ma tylko sizedir.
 * \todo wstawić dynamiczne stringi z realokacją (a w zasadzie znaleźć do tego bibliotekę)
 * \todo zrobić sensowniejsze określanie rozmiaru okna, trzeba by przed wywołaniem rekurencyjnym
 *      przelecieć katalog, do którego wchodzimy
 */
void filename_dialog(char* title, char** selected_file, char* directory) {
	struct dirent **namelist;
	int list_size, i;
	
	newtCenteredWindow(20, 20, directory);
	newtComponent directory_listing = newtListbox(0, 0, 20, NEWT_FLAG_RETURNEXIT);
	newtComponent directory_form = newtForm(NULL, NULL, 0);
	newtFormAddComponent(directory_form, directory_listing);
  list_size = scandir(directory, &namelist, 0, alphasort);
	for(i=0; i < list_size; ++i) {
		newtListboxAppendEntry(directory_listing, namelist[i]->d_name, namelist[i]);
	};
	newtListboxSetWidth(directory_listing, 20);  // do poprawki
	newtRunForm(directory_form);
	if( ((struct dirent*) newtListboxGetCurrent(directory_listing))->d_type == DT_DIR) {
		/* jeżeli wybrano katalog, to zrób stringa z odpowiednią ścieżką i wywołaj rekurencyjnie filename_dialog */
		char* new_dir = malloc(strlen(directory) + strlen(((struct dirent*) newtListboxGetCurrent(directory_listing))->d_name) + 3);
		sprintf(new_dir, "%s/%s", directory, ((struct dirent*) newtListboxGetCurrent(directory_listing))->d_name);
		newtPushHelpLine(new_dir);
		filename_dialog(title, selected_file, new_dir);
			free(new_dir);
	} else {
		/* jeżeli wybrano plik, to koniec przetwarzania, trzeba tylko przerobić scieżkę na absolutną za pomocą realpath() */
		char* path_to_process = (char*) malloc(strlen( ( (struct dirent*) newtListboxGetCurrent(directory_listing) )->d_name ) + 1) ;
		sprintf(path_to_process, "%s/%s", directory, ((struct dirent*) newtListboxGetCurrent(directory_listing))->d_name);
		*selected_file = (char*) malloc(PATH_MAX);
		realpath(path_to_process, *selected_file);
		free(path_to_process);
		newtPushHelpLine(*selected_file);
	};
	free(namelist);
	newtFormDestroy(directory_form);
	newtPopWindow();
	newtRefresh();
}

/*! menu pojedynczego wyboru
 *
 * wyświetla na ekranie nowe okno z listą możliwości do wyboru, zwraca
 * numer wybranej przez użytkownika pozycji (zaczynając od zera).
 * Uwaga: nie sprawdzamy, czy menu zmieści się na ekranie!
 */
int single_choice_menu(int no_pos, char* menuItems[], char* menuTitle) {
  int j, x, maxWidth;
  /* wyznaczamy niezbędną szerokość okna menu */
  maxWidth = (menuTitle == NULL) ? 0 : strlen(menuTitle)+6;
  for(j=0; j < no_pos; ++j)
    if((x = strlen(menuItems[j])) > maxWidth)
     maxWidth = x;
  newtCenteredWindow(maxWidth, no_pos, menuTitle);
  newtComponent menuForm = newtForm(NULL, NULL, 0);
  newtComponent menuList = newtListbox(0, 0, no_pos, NEWT_FLAG_RETURNEXIT);
  int liczby[no_pos];
  /* ta niewątpliwie piękna konstrukcja wynika z faktu, że newt lubi wskaźniki
   * do voidów, zamiast po prostu zwracania pozycji. */ 
  for(j = 0; j < no_pos; ++j) {
    liczby[j] = j;
    newtListboxAppendEntry(menuList, menuItems[j], &(liczby[j]));
  };
  newtListboxSetWidth(menuList, maxWidth);
  newtFormAddComponent(menuForm, menuList);
  newtRunForm(menuForm);
  x = *((int*) newtListboxGetCurrent(menuList));
  newtFormDestroy(menuForm);
  newtPopWindow();
  return x;
  };
