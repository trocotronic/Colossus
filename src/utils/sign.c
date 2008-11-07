#include <openssl/rsa.h>       /* SSL stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/ripemd.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _WIN32
#define O_BINARY 0x0
#include <unistd.h>
#endif

char *pk = "-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEAzpj3toI/45bewlCgT0dy+dZsY1eyUZrn8PfIBZG7Pom8Leph\n"
"0T/L+TuQu+XFpeh/t+VgTFfBNGO7Xqw5aiYw7vwtAOn5V2+acyjFKq435WDvBTMS\n"
"8Qo2V5c9h0W3LwrkJHRuTiyaTq2CeX6dAE19buEpZZM2qkzkN3G6F1yZ37iC7WDB\n"
"AtxXlsjdSGuR+xWlXL+6N6HLwqyI06dI2tvtklq6zUP+o29L/jGSi5Vt6FCmmoiN\n"
"KP7WHq+cs5EqEobwInhLFk99ajhoakTXjpq613DwCK1Uz3rfAtHXyG1ietvaXdBB\n"
"rEO3WzRLxxKWKssQRt2Nj34GumzVHh5TRYB9nQIDAQABAoIBAQCpAWHDH5H8MDNS\n"
"anp54E0iLFLGgbsSHtKWwVwTkj/GzQ3v0cjrUHEgFaY0z6Z8LlIssauxSCh5Hwzs\n"
"SZ8+QrfNCOYX1U9wQ4/pnPSOEa8QKdfePQXFwUDrLoHa4yETjqlLWSPTN5GTw0T0\n"
"9qqg0MNHrVIcEe2qKvSWlqZQ7iPCasgO5yfqHOkEHohHi61rtbNP3CaN08IQZECv\n"
"JA0vGWrD6F9quSUyUOXbGtOvuimx+h10OwEnmtGjXQ9ZdBdxsQyFq5BIm2VB8xOG\n"
"j2ruhDYZ5XpdPqE0z6GveQe9zYyeZ6EGabN+z66IFtWJIJ0yJ8RSDAVbU8sdSIzy\n"
"Z8iXIfRRAoGBAP7fjPQGWx30NoTo1ce1RasXddrc/nq0jBFjDh06B9zhoGELRIg/\n"
"XiiZ5cUlqWGszVImf7/lxbaFxjh/wuULHE/4+vjHTF2kmangJpcubIhYlIKtiYUM\n"
"oLJQqhnW+5j8gyBP88CC0jc9xu95BPuP3MQn28wQRoQESKnHZuzaSEKvAoGBAM+C\n"
"yBkTU1nBtQd+Qp+MaTRAXP/VhIIuILDM9qBL27cpT8ZT4e8Wbwi1cxjjl7HK14HX\n"
"3nM2N02Popy6GhfZNfSvw3ecSwvroL4mUx0bhPKJuydDrhq7axNuPbYAxpl50ZOT\n"
"m2bli8uGDTPQCfHXeVDeUBav3AwTMvYXpOCvNEdzAoGAQv/pQczZ7wnfuxip+hHA\n"
"+rT0GlC15PPJTljHwQ8cOghl7JzVqytdSTcLm8PGvxJ59vp/4qY4Tz7jWL7dMPC2\n"
"xJ8i+nsJrCQ08N8nxd5CUaVXhPKxj/Q92iIyVRCamyDmJ3xdC2JYeIUY4qLhmG+9\n"
"DSOdOAufPd0SbO8qM2E+VakCgYEAwbi4ESDHiV2bIPmwPL6aYFtN9tBgOh/SCPvv\n"
"qcnnvmBkxyP8InXxBlJOtweR0DsrYV4jn68XheL3zhS201jGVD3Z30objW9Vyu6A\n"
"XQYZ4UrPW2KFoRMibStXlRe4UAM3sev1AeR902y72oj3H70m1mYUonleli8+Phvo\n"
"opEt3x0CgYEA9ez2walBf+e5G1hJwo0sAmnTRgU8q4FMmpFvdolIUL4iR9b/Yjrs\n"
"XxQxU+6Km6QErS5gyZHIcrHrylQpc212h/+fOwXwmMTo5qu6I3KgRsH+P4JSCrcJ\n"
"xnZNOIDg/qRamB7UWoSwrVvX10KSjy+7jpuS8kklMxEMUUG0zx6sgH8=\n"
"-----END RSA PRIVATE KEY-----";

int main(int argc, char *argv[])
{
	int fd;
	RSA *pvkey = NULL;
	BIO *bio = NULL;
	char *c, *sgn, *dgs;
	struct stat sb;
	unsigned int sgnlen, len, dgslen;
	EVP_MD_CTX ctx;
	if (argc < 2)
	{
		printf("Faltan parametros: %s <archivo> [out]", argv[0]);
		return 1;
	}
	if (!(bio = BIO_new_mem_buf(pk, strlen(pk))))
	{
		printf("No se puede cargar la clave");
		return 1;
	}
	if (!(pvkey = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL)))
	{
		printf("No se puede formatear la clave");
		return 1;
	}
	if (!(fd = open(argv[1], O_RDWR|O_BINARY)))
	{
		printf("No se puede abrir %s", argv[1]);
		return 1;
	}
	fstat(fd, &sb);
	c = (char *)malloc(sizeof(char) * (sb.st_size + 1));
	read(fd, c, sb.st_size);
	OpenSSL_add_all_digests();
	EVP_DigestInit(&ctx, EVP_get_digestbyname("SHA1"));
	EVP_DigestUpdate(&ctx, c, sb.st_size);
	dgslen = EVP_MAX_MD_SIZE;
	dgs = (char *)malloc(sizeof(char) * dgslen);
	EVP_DigestFinal(&ctx, dgs, &dgslen);
	sgnlen = RSA_size(pvkey);
	sgn = (char *)malloc(sizeof(char) * sgnlen);
	if (RSA_sign(NID_sha1, dgs, dgslen, sgn, &sgnlen, pvkey))
	{
		lseek(fd, 0, SEEK_SET);
		write(fd, "\x79\xF3\xA3\x81\xA9\xDB\x30\xDF\x7E\x1A\x4F\x32\xC7\x5D\x88\x2A", 16);
		write(fd, sgn, sgnlen);
		write(fd, c, sb.st_size);
	}
	else
		printf("No se ha podido firmar %s", ERR_error_string(ERR_get_error(), NULL));
	free(c);
	free(sgn);
	free(dgs);
	close(fd);
	return 0;
}
