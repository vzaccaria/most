/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________
 *     /  |/  / __ \/ ___/_  __/
 *    / /|_/ / / / /\__ \ / /
 *   / /  / / /_/ /___/ // /
 *  /_/  /_/\____//____//_/
 *
 * Multi-Objective System Tuner
 * Copyright (c) 2007-2011 Politecnico di Milano
 *
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *
 * This file is confidential property of Politecnico di Milano.
 *
 * @STSHELL_LICENSE_END@ */
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <unistd.h>
#include <string.h>
#include <string>

using namespace std;

#include <most_license_public_key.h>

#define MAX_DIGEST_SIZE 512

unsigned char md_value[EVP_MAX_MD_SIZE];
unsigned int md_len;

unsigned char signed_digest[MAX_DIGEST_SIZE];
unsigned int signed_digest_length;

int digest_read(char *file_name, unsigned char *dest, unsigned int *length) {
  BIO *bio, *b64, *bio_out;
  FILE *my_file = fopen(file_name, "r");
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new_fp(my_file, BIO_NOCLOSE);
  bio = BIO_push(b64, bio);
  *length = BIO_read(bio, dest, MAX_DIGEST_SIZE);
  BIO_free_all(bio);
  return *length;
}

int digest_read_from_memory(const char *buf, unsigned char *dest,
                            unsigned int *length) {
  BIO *bio, *b64, *bio_out;
  char *buffer = strdup(buf);
  b64 = BIO_new(BIO_f_base64());
  /* bio = BIO_new_fp(my_file, BIO_NOCLOSE); */
  bio = BIO_new_mem_buf(buffer, -1);
  bio = BIO_push(b64, bio);
  *length = BIO_read(bio, dest, MAX_DIGEST_SIZE);
  BIO_free_all(bio);
  free(buffer);
  return *length;
}

RSA *read_public_key(BIO *from_where) {
  EVP_PKEY *pkey = NULL;
  RSA *rsa = NULL;

  pkey = PEM_read_bio_PUBKEY(from_where, NULL, NULL, NULL);

  if (pkey) {
    rsa = EVP_PKEY_get1_RSA(pkey);
    if (rsa)
      RSA_up_ref(rsa);
    else {
      printf("ERROR: RSA_getFAILED.\n");
      return NULL;
    }
    EVP_PKEY_free(pkey);
  } else {
    printf("ERROR: PEM_read_bio_RSAPublicKey FAILED.\n");
    return NULL;
  }
  return rsa;
}

#define MAX_MESSAGE_SIZE 8192
unsigned char the_message[MAX_MESSAGE_SIZE];

void compute_digest_of_clear_text(const char *clear_t) {
  EVP_MD_CTX *mdctx;
  int i;
  int l;
  const EVP_MD *md;

  char *clear_text = strdup(clear_t);

  md = EVP_sha256();
  mdctx = EVP_MD_CTX_create();
  EVP_MD_CTX_init(mdctx);
  EVP_DigestInit_ex(mdctx, md, NULL);
  EVP_DigestUpdate(mdctx, clear_text, strlen(clear_text));
  EVP_DigestFinal_ex(mdctx, md_value, &md_len);
  EVP_MD_CTX_free(mdctx);
  free(clear_text);

  /*
  printf("Digest is: ");
  for(i = 0; i < md_len; i++) printf("%02x", md_value[i]);
  printf("\n");
  */
}

void st_initialize_ossl() {
  OpenSSL_add_all_ciphers();
  OpenSSL_add_all_digests();
}

bool st_verify_signature(string clear_text, string signature) {
  BIO *pkeymem;
  RSA *pkeyrsa;

  pkeymem = BIO_new_mem_buf(pkey, -1);
  pkeyrsa = read_public_key(pkeymem);

  BIO_free_all(pkeymem);
  digest_read_from_memory(signature.c_str(), signed_digest,
                          &signed_digest_length);
  compute_digest_of_clear_text(clear_text.c_str());
  return (RSA_verify(NID_sha256, md_value, md_len, signed_digest,
                     signed_digest_length, pkeyrsa) == 1);
}

/*
int main(char **argv, int argc)
{
    read_file_and_compute_digest("lic.vittorio");
    printf("RSA Result : %d\n",
            RSA_verify(NID_sha256, md_value, md_len, signed_digest,
signed_digest_length, pkeyrsa));
}
*/

#define MOST_MAGIC 10071979

#define PRE_ERR "Sign: "

#include <iostream>

bool st_sign_run(string command) {
  string sh_com = "/bin/bash -c \"(" + command + ")\"";
  int rs = system(sh_com.c_str());
  if (rs == -1 || rs == 127) {
#if defined(DEBUG)
    cout << PRE_ERR << sh_com << " : failed - " << rs << endl;
    flush(cout);
#endif
    return false;
  }
  /** The upper 8 bits contain the exit status */
  rs = rs >> 8;
#if defined(DEBUG)
  cout << PRE_ERR << sh_com << " : " << ((rs) ? "FAIL" : "OK") << endl;
  flush(cout);
#endif
  if (rs)
    return false;
  return true;
}

#include <iostream>
#include <sstream>

string tmp_file() {
  ostringstream dir;
  dir << "./.most_tmp_f_" << getpid() << ".int";
  return dir.str();
}

#define UI(e) ((unsigned char)e)
#define HID(v) (UI(v[5]) * UI(v[4]) * UI(v[3]) * UI(v[2]))

long int st_get_hw_addr_from_s(string s) {
  int v[6];
  sscanf(s.c_str(), "%2x:%2x:%2x:%2x:%2x:%2x", &v[0], &v[1], &v[2], &v[3],
         &v[4], &v[5]);
  long int newhostid = HID(v);
  return newhostid;
}

#include <fstream>
#include <net/if.h>
#include <stdio.h>
#include <sys/ioctl.h>
/* #include <sys/sockio.h> */
#include <sys/socket.h>
#include <sys/types.h>

long int st_get_hw_addr(string &addr) {
  int sockfd;
  int io;
  char buffer[1024];
  struct ifreq ifr;

  addr = "N/A";

  sprintf(ifr.ifr_name, "eth0");

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    cout << "Can't derive sockfd" << endl;
    return 0;
  }

  io = ioctl(sockfd, SIOCGIFADDR, (char *)&ifr);

  if (io < 0) {
    cout << "Can't issue ioctl" << endl;
    return 0;
  }

  sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
          (unsigned char)ifr.ifr_ifru.ifru_addr.sa_data[0],
          (unsigned char)ifr.ifr_ifru.ifru_addr.sa_data[1],
          (unsigned char)ifr.ifr_ifru.ifru_addr.sa_data[2],
          (unsigned char)ifr.ifr_ifru.ifru_addr.sa_data[3],
          (unsigned char)ifr.ifr_ifru.ifru_addr.sa_data[4],
          (unsigned char)ifr.ifr_ifru.ifru_addr.sa_data[5]);
  addr = buffer;
  long int newhostid = HID(ifr.ifr_ifru.ifru_addr.sa_data);
  return newhostid;
}

/* The following is obsolete */
long int st_get_host_id() {
  long int id, res;
  /* get real (default) hostid */
  id = gethostid();
#if defined(_MOST_DEBUG_LICENSE_)
  cout << "Actual hostid: " << id << endl;
#endif
  return id;
}

key_t st_get_unique_most_key() {

  unsigned int hid =
      (unsigned int)((unsigned long int)st_get_host_id()) & 0x00000000FFFFFFFF;
  unsigned int key = hid ^ MOST_MAGIC;
  return key;
}

int st_code(int val) { return (MOST_MAGIC * 2) ^ val; }

bool st_load_data(string &file_name, string &script) {
  FILE *file = fopen(file_name.c_str(), "r");
  if (!file) {
    return false;
  }

  char line[1000];

  int n = 0;
  bool clear = true;
  while (fgets(line, sizeof(line) - 1, file)) {
    script = script + string(line);
  }
  fclose(file);
  return true;
}

bool st_verify_script(string &file_name, string &key_name) {
  string the_file;
  string the_key;

  if (!st_load_data(file_name, the_file))
    return false;

  if (!st_load_data(key_name, the_key))
    return false;

  if (!st_verify_signature(the_file, the_key))
    return false;

  return true;
}
