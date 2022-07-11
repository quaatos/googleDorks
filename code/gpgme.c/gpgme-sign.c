/* Sign data. All the information is *in* the source code: the data to
   sign, the key holder's name, the passphrase... */

#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gpgme.h>

#define fail_if_err(err)					\
  do								\
    {								\
      if (err)							\
        {							\
          fprintf (stderr, "%s:%d: %s: %s\n",			\
                   __FILE__, __LINE__, gpgme_strsource (err),	\
		   gpgme_strerror (err));			\
          exit (1);						\
        }							\
    }								\
  while (0)

#define PASSPHRASE "abc\n"

gpgme_error_t
passphrase_cb(void *opaque, const char *uid_hint, const char *passphrase_info,
	      int last_was_bad, int fd)
{
	write(fd, PASSPHRASE, strlen(PASSPHRASE));
	return 0;
}

#define KEYRING_DIR "/var/tmp/mycrypto"

#define SENTENCE "I swear it is true"

#define MAXLEN 4096

int
main(int argc, char **argv)
{
	gpgme_error_t   error;
	gpgme_engine_info_t info;
	gpgme_ctx_t     context;
	gpgme_key_t     signing_key;
	gpgme_data_t    clear_text, signed_text;
	gpgme_sign_result_t result;
	gpgme_user_id_t user;
	char           *buffer;
	ssize_t         nbytes;

	/* Initializes gpgme */
	gpgme_check_version(NULL);

	/* Initialize the locale environment.  */
	setlocale(LC_ALL, "");
	gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifdef LC_MESSAGES
	gpgme_set_locale(NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif

	error = gpgme_new(&context);
	fail_if_err(error);
	/* Setting the output type must be done at the beginning */
	gpgme_set_armor(context, 1);

	/* Check OpenPGP */
	error = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
	fail_if_err(error);
	error = gpgme_get_engine_info(&info);
	fail_if_err(error);
	while (info && info->protocol != gpgme_get_protocol(context)) {
		info = info->next;
	}
	/* TODO: we should test there *is* a suitable protocol */
	fprintf(stderr, "Engine OpenPGP %s is installed at %s\n", info->version,
		info->file_name);
	/* And not "path" as the documentation says */

	/* Initializes the context */
	error = gpgme_ctx_set_engine_info(context, GPGME_PROTOCOL_OpenPGP, NULL,
					  KEYRING_DIR);
	fail_if_err(error);

	error = gpgme_op_keylist_start(context, "John Smith", 1);
	fail_if_err(error);
	error = gpgme_op_keylist_next(context, &signing_key);
	fail_if_err(error);
	error = gpgme_op_keylist_end(context);
	fail_if_err(error);
	error = gpgme_signers_add(context, signing_key);
	fail_if_err(error);

	user = signing_key->uids;
	printf("Signing for %s <%s>\n", user->name, user->email);

	/* Prepare the data buffers */
	error = gpgme_data_new_from_mem(&clear_text, SENTENCE, strlen(SENTENCE), 1);
	fail_if_err(error);
	error = gpgme_data_new(&signed_text);
	fail_if_err(error);

	/* Get the passphrase */
	gpgme_set_passphrase_cb(context, passphrase_cb, NULL);

	/* Sign */
	error =
	    gpgme_op_sign(context, clear_text, signed_text, GPGME_SIG_MODE_NORMAL);
	fail_if_err(error);
	result = gpgme_op_sign_result(context);
	if (result->invalid_signers) {
		fprintf(stderr, "Invalid signer found: %s\n",
			result->invalid_signers->fpr);
		exit(1);
	}
	if (!result->signatures || result->signatures->next) {
		fprintf(stderr, "Unexpected number of signatures created\n");
		exit(1);
	}

	/* Sign */
	error = gpgme_data_seek(signed_text, 0, SEEK_SET);
	fail_if_err(error);
	buffer = malloc(MAXLEN);
	nbytes = gpgme_data_read(signed_text, buffer, MAXLEN);
	if (nbytes == -1) {
		fprintf(stderr, "%s:%d: %s\n",
			__FILE__, __LINE__, "Error in data read");
		exit(1);
	}
	buffer[nbytes] = '\0';
	printf("Signed text (%i bytes):\n%s\n", (int) nbytes, buffer);
	/* OK */

	exit(0);
}
