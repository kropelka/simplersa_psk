#ifndef RSA_KEYGEN_H
#define RSA_KEYGEN_H

int rsa_keygen(int no_bits, mpz_t* n, mpz_t* d, mpz_t* e, int long_public);
int rsa_key_to_plaintext(char filename[], mpz_t* n, mpz_t* d, int key_type);
int rsa_plaintext_to_key(char filename[], mpz_t* n, mpz_t* d, mpz_t* e, int* key_type);
#endif
