#include <stdio.h>
#include <gmp.h>
#include <fcntl.h>
#include <time.h>
#include <curses.h>

#define NR_TESTS 25  /* liczba uzyc algorytmu Millera-Rabina */
         /* do sprawdzenia pierwszosci wygenerowanej */
         /* liczby losowej */

#define PLAINTEXT_BASE 16 /* podstawa zapisu klucza w plaintekscie */
gmp_randstate_t generator;



/*! Generowanie losowej liczby pierwszej.
 *
 * Ponieważ zdecydowana większość liczb pierwszych jest
 * nieparzysta, to generujemy liczbę losową (n-1) bitową,
 * po czym poddajemy ją przekształceniu x |-> 2x+1 aby
 * otrzymać nieparzystą liczbę n-bitową.
 * \param p wskaźnik na liczbę
 * \param bits liczba bitów generowanej liczby pierwszej
 */
void random_prime(mpz_t* p, int bits) {
  int tries = 0;
  int is_prime = 0;
  while(!is_prime) {
    ++tries;
    mpz_urandomb(*p, generator, bits-1);
    mpz_mul_ui(*p, *p, 2); /* generujemy nieparzysta*/
    mpz_add_ui(*p, *p, 1); /* liczbe */
    is_prime = mpz_probab_prime_p(*p, NR_TESTS);
  };
//  printw("Poszukiwanie losowej liczby pierwszej wymagało %d prób. \n", tries);
}

unsigned int random_seed() {
  FILE* urandom = fopen("/dev/urandom", "r");
  unsigned int seed;
  if(urandom == NULL) {
    fprintf(stderr,"Błąd otwarcia /dev/urandom, otrzymany klucz NIE JEST BEZPIECZNY.\n");
    return time(0);
  }
  else {
    fread(&seed, sizeof(seed), 1, urandom);
    return seed;
  };
  fclose(urandom);
}


/*! \brief inicjalizacja generatora liczb losowych
 *
 *
 * \param type_of_generator wybor rodzaju generatora
 */



void initialize_generator(int type_of_generator, unsigned int seed) {
    switch(type_of_generator) {
      case 0:
        gmp_randinit_default(generator);
        break;
      case 1:
        gmp_randinit_mt(generator);
        break;
    };
    gmp_randseed_ui(generator, seed);
}

/*! \brief generowanie klucza RSA
 *
 * Funkcja generuje klucz RSA o zadanej liczbie bitów.
 * Zgodnie z założeniami RSA dostajemy (n,e) oraz (n,d).
 * \param no_bits liczba bitów generowanego klucza
 * \param n pierwszy składnik klucza
 * \param e składnik e klucza publicznego
 * \param d składnik d klucza prywatnego
 * \param long_public określa, czy wygenerować długi składnik e klucza publicznego
 * \todo Należy zadbać o to, aby zgodnie ze standardem obie liczby pierwsze były blisko siebie.
 */

void rsa_keygen(int no_bits, mpz_t* n, mpz_t* d,mpz_t* e, int long_public) {
  initialize_generator(0, random_seed());
  mpz_t p, q, phi, gcd;
  mpz_init(gcd);
  mpz_init2(p, no_bits/2);
  mpz_init2(q, no_bits/2);
  mpz_init2(phi, no_bits);
  random_prime(&p, no_bits/2);
  random_prime(&q, no_bits/2);
  //gmp_printf("p = %Zd, q = %Zd\n", p, q);
  mpz_mul(*n, p, q);    /* n = p*q */
  mpz_sub_ui(p, p, 1);
  mpz_sub_ui(q, q, 1);
  mpz_mul(phi,p,q);  /* phi(n) = (p-1)*(q-1) */
  if(long_public) {
    do {
      mpz_urandomb(*e, generator, no_bits);
      mpz_gcd(gcd, *e, phi);
    } while( mpz_cmp_d(gcd, 1));
  } else {
      mpz_set_ui(*e, 1);
      do {
        mpz_add_ui(*e, *e, 1);
        mpz_gcd(gcd, *e, phi);
      } while( mpz_cmp_d(gcd, 1));
  }
  mpz_invert(*d, *e, phi); /* d = e^-1 mod phi */
  mpz_clear(p);
  mpz_clear(q);
  mpz_clear(phi);
  mpz_clear(gcd);
}

/*! wczytanie klucza z pliku tekstowego
 *
 * Próbuje wczytać z pliku filename klucz RSA. Zwraca kod błędu 1, jeśli
 * format pliku jest nieprawidłowy, oraz 2, jeśli plik nie istnieje bądź
 * nie da się go otworzyć.
 */
int rsa_plaintext_to_key(char filename[], mpz_t* n, mpz_t* d, mpz_t* e, int* key_type)
 {
  FILE* keyfile = fopen(filename, "r");
  if(keyfile != NULL) {
    mpz_t nn, dd;
    mpz_init(nn);
    mpz_init(dd);
    gmp_fscanf(keyfile, "%d %Zx %Zx", key_type, nn, dd);
    if(!mpz_sgn(nn) || !mpz_sgn(dd))
      return 1;
    mpz_set(*n, nn);
          if(*key_type == 1) { 
            mpz_set(*e, dd);
          } else
            mpz_set(*d, dd);
  } else
    return 2;
  return 0;
}

int rsa_key_to_plaintext(char filename[], mpz_t* n, mpz_t* d, int key_type) {
  FILE* keyfile;
  if((keyfile = fopen(filename,"r")) != NULL) {
                fclose(keyfile);
    return 2;
  } else {
          keyfile = fopen(filename, "w");
          fprintf(keyfile, "%d ", key_type);
    mpz_out_str(keyfile, PLAINTEXT_BASE, *n);
    fprintf(keyfile, " ");
    mpz_out_str(keyfile, PLAINTEXT_BASE, *d);
          fclose(keyfile);
    return 0;
  };
}
