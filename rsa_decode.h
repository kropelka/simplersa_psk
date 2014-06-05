#ifndef RSA_DECODE_H
#define RSA_DECODE_H
void rsa_encode_file(FILE* input_file, FILE* output_file, mpz_t* n, mpz_t* e, int block_size, int out_block_size, int verbose);
void rsa_decode_file(FILE* input_file, FILE* output_file, mpz_t* n, mpz_t* d, int block_size, int out_block_size, int verbose);
#endif
