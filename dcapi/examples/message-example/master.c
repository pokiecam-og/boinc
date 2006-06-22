/**************************************************
 * Message-examle Master program
 *
 * Created by: Gabor Vida
 *
 * The aim of the program is to test message passing between the master and
 * the client in a Public Resource Computing environment. The
 * program follows the client server model, therefore having a server-side and
 * a client-side part.
 *
 * This is the server-side part of the program. It creates a WorkUnit (WU) for
 * a Boinc project using the DC Application Programming Interface.  The program
 * takes a text file (input.txt) and submits it into the PRC-infrastructure,
 * where it get processed by a client. After the submittion, the master waits
 * for a user interaction (press ENTER), when the client downloaded the WU.
 * After this, the master sends a sample message to the client, which is
 * running around 5 mins on the client. If everything goes well, the client
 * will get the message, and sends back a reply. After the client has returned
 * the results of the WU, the program assimilates it.
 *
 *****************************************************/

#include <dc.h>

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#define INPUT_LABEL		"in.txt"
#define OUTPUT_LABEL		"out.txt"

/* Number of WUs we have created */
static int created_wus;
/* Number of results we have received so far */
static int processed_wus;

/* Command line options */
static const struct option longopts[] =
{
	{ "config",	required_argument,	NULL,	'c' },
	{ "help",	no_argument,		NULL,	'h' },	
	{ NULL }
};

static void process_result(DC_Workunit *wu, DC_Result *result)
{
	char *tag;

	/* Internal housekeeping of the number of results we have seen */
	processed_wus++;

	/* Extract our own private identifier */
	tag = DC_getWUTag(wu);

	DC_log(LOG_NOTICE, "Work unit %s has completed", tag);

	/* We no longer need the work unit */
	DC_destroyWU(wu);

	free(tag);
}

static void process_subresult(DC_Workunit *wu, const char *logicalFileName, const char *path)
{
	/* There is no subresult in this example */
}

static void process_message(DC_Workunit *wu, const char *message)
{
	char *tag;

	tag = DC_getWUTag(wu);
	DC_log(LOG_NOTICE, "Message '%s' has arrived from work unit %s", message, tag);

	free(tag);
}

static void create_work(void)
{
	char ch;
	DC_Workunit *wu;

	wu = DC_createWU("message-example", NULL, 0, "message-test");
	if (!wu)
	{
		DC_log(LOG_ERR, "Work unit creation has failed");
		exit(1);
	}

	if (DC_addWUInput(wu, INPUT_LABEL, "input.txt",	DC_FILE_PERSISTENT))
	{
		DC_log(LOG_ERR, "Failed to register WU input file");
		exit(1);
	}

	if (DC_addWUOutput(wu, OUTPUT_LABEL))
	{
		DC_log(LOG_ERR, "Failed to register WU output file");
		exit(1);
	}

	if (DC_submitWU(wu))
	{
		DC_log(LOG_ERR, "Failed to submit WU");
		exit(1);
	}

	created_wus++;

	printf("Press ENTER, after this WU is downloaded by a BOINC client");
	ch = getchar();

	DC_sendWUMessage(wu, "This is a test message");
}

static void print_help(const char *prog) __attribute__((noreturn));
static void print_help(const char *prog)
{
	const char *p;

	/* Strip the path component if present */
	p = strrchr(prog, '/');
	if (p)
		prog = p;

	printf("Usage: %s {-c|--config} <config file>\n", prog);
	printf("Available options:\n");
	printf("\t-c <file>, --config <file>\tUse the specified config. file\n");
	printf("\t-h, --help\t\t\tThis help text\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	char *config_file = NULL;
	int c;

	while ((c = getopt_long(argc, argv, "ch", longopts, NULL)) != -1)
	{
		switch (c)
		{
			case 'c':
				config_file = optarg;
				break;
			case 'h':
				print_help(argv[0]);
				break;
			default:
				exit(1);
		}
	}

	if (optind != argc)
	{
		fprintf(stderr, "Extra arguments on the command line\n");
		exit(1);
	}

	/* Specifying the config file is mandatory, otherwise the master can't
	 * run as a BOINC daemon */
	if (!config_file)
	{
		config_file = strdup("dc-api.conf");
		fprintf(stdout, "You didn't specified the config file, use 'dc-api.conf'\n");
	}

	/* Initialize the DC-API */
	if (DC_initMaster(config_file))
	{
		fprintf(stderr, "Master: DC_initMaster failed, exiting.\n");
		exit(1);
	}

	/* We need the result callback function only */
	DC_setMasterCb(process_result, process_subresult, process_message);

	DC_log(LOG_NOTICE, "Master: Creating work unit");
	create_work();
	DC_log(LOG_NOTICE, "Master: Work unit has been created. Waiting "
		"for result/message ...\n");

	/* Wait for the work units to finish */
	while (processed_wus < created_wus)
		DC_processMasterEvents(600);

	return 0;
}
