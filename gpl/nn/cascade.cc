#define PRINT_STDOUT // macro che se definita permette di stampare a video i
                     // risultati di cascade
//#define PRINT_REPORT	//macro che se definita permette di stampare un file di
// report per ogni modello

#define TRAINING_ALGORITHM                                                     \
  FANN_TRAIN_RPROP // algortimo di apprendimento utilizzato (dalla versione 3.0
                   // di FANN potrebbe esserci Levenberg-Marquardt)
#define TEST_DATA_PERCENTAGE                                                   \
  20 // percentuale di dati del dataset da dedicare al testing

#define MODEL_THREADS                                                          \
  1 // numero di threads che eseguiranno la funzione "cascade_engine". Ovvero
    // numero di modelli da apprendere in parallelo
#define SHUFFLE_THREADS                                                        \
  1 // numero di threads che eseguiranno la funzione "compute_shuffle" per ogni
    // modello da apprendere.

#include <fann.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <getopt.h>
#include <ctype.h>

// Struttura dati che definisce due matrici: una contenente gli input di cui si
// vogliono calcolare le uscite, l'altra contenente le uscite calcolate dalla
// rete neurale. Questa struttura viene inizializzata dalla funzione
// "datafile_parser" che dopo aver analizzato il file del dataset di ingressi
// alloca la memoria necessaria a contenere la struttura. La struttura viene
// riempita da "compute_outputdata" ed infine viene salvata su file da
// "writefile".
typedef struct Matrix {
  fann_type **input;
  fann_type **output;
} Matrix;

// Struttura dati utilizzata come ingresso alla funzione del thread che esegue
// gli shuffle ("compute_shuffle"). Contiene alcune variabili che vengono
// utilizzate come ingressi (PARAMETRI DI INPUT) ed alcune variabili che vengono
// aggiornate di volta in volta ad ogni chiamata della funzione
// "compute_shuffle"
// e che verranno interpretate come variabili di output (PARAMETRI DI OUTPUT).
// La struttura contiene inoltre alcune variabili per la gestione del buffer da
// utilizzare per il report (PARAMETRI DI REPORT)
typedef struct shuffle_in {

  // PARAMETRI DI INPUT
  struct fann_train_data *all_data; // struttura che memorizza tutti i dati da
                                    // utilizzare per l'apprendimento
  unsigned int
      nn_next_shuffle_best_net; // centro dell'intorno di ricerca
                                // (numero di neuroni dell'hidden layer)
  fann_type min; // valore min dei dati forniti per la denormalizzazione
  fann_type max; // valore max dei dati forniti per la denormalizzazione
  unsigned int thread_number; // numero identificativo del thread

  // PARAMETRI DI OUTPUT
  unsigned int nn_shuffle_best_net; // numero di neuroni dell'hidden layer della
                                    // miglior rete trovata
  fann_type e_test_best_net_shuffle; // miglior errore di testing relativo alla
                                     // miglior rete trovata
  struct fann *best_net_shuffle; // struttura utilizzata per salvare la miglior
                                 // rete trovata

// PARAMETRI DI REPORT
#ifdef PRINT_REPORT
  char *buffer_session;  // buffer utilizzato per memorizzare temporaneamente i
                         // messaggi di report
  int lunghezza_tot;     // parametro di servizio per tener traccia del numero
                         // totale dei caratteri inseriti nel buffer
  int dimensione_buffer; // parametro di servizio indicante la dimensione
                         // allocata per il buffer
#endif
} shuffle_in;

// PROTOTIPI DELLE FUNZIONI

//	THREAD Function
// Funzione principale del programma che implementa la ricerca del miglior
// modello per le coppie ingresso/i-uscita/e fornite in ingresso Parametri di
// ingresso:	par = identifica il numero del thread, quindi il modello che
// quel particolare thread sta elaborando Parametro di uscita:		void. La
// funzione non ritorna nulla, ma salva su disco due file: "model"_bestnet.net
// contenente la miglior rete trovata e
// "model"_report.txt contenente un report sull'intero processo di
// apprendimento.
void *cascade_engine(void *par);

//	THREAD Function
// Funzione che implementa la ricerca della miglior rete nell'intorno fin a quel
// momento trovato, mischiando i dati forniti. Parametri di ingresso:	par =
// istanza della struttura "shuffle_in" con "PARAMETRI DI INPUT" e "PARAMETRI DI
// REPORT" inizializzati Parametro di uscita:		void. La funzione non
// ritorna nulla, ma salva i risultati dell'elaborazione nei PARAMETRI DI OUTPUT
// della struttura 				passatagli in ingresso (par).
void *compute_shuffle(void *par);

// Funzione che calcola i valori minimo e massimo delle uscite del dataset
// passato in ingresso
// Parametri di ingresso:	dati = dataset di cui si vogliono calcolare i
// valori minimo e massimo delle uscite
// Parametro di uscita:		puntatore ad un array di due elementi con il
// valore min in pos 0 e il valore massimo in pos 1
fann_type *compute_minmax(struct fann_train_data *dati);

// Funzione che calcola l'errore percentuale medio utilizzando la rete calcolata
// da cascade2 Parametri di ingresso:	ann = struttura della rete neurale da
// utilizzare per il calcolo dell'errore testing_data = dataset su cui calcolare
// l'errore 				min, max = valori minimo e massimo
// utilizzati per la denormalizzazione Parametro di uscita:		errore
// percentuale medio pesato
fann_type compute_error(struct fann *ann, struct fann_train_data *testing_data,
                        fann_type min, fann_type max);

// Funzione che calcola il numero di dati di test da utilizzare come percentuale
// sui dati totali (es #test = 20% dei dati tot) Parametri di ingresso:
// total_data_number = numero di dati totali a disposizione per l'apprendimento
//				percent_value = valore percentuale
// Parametro di uscita:		numero di dati di test
unsigned int compute_test_data_number(unsigned int total_data_number,
                                      unsigned short percent_value);

// Funzione che inizializza i parametri di apprendimento in base al tipo di
// constraint passato dall'utente Parametri di ingresso:	constraint =
// tipo di constraint da utilizzare per l'apprendimento (scelto tra LOW, MEDIUM,
// HIGH, FAST) Parametro di uscita:		void.
void init_constr_params(char *constraint);

// Funzione che inizializza i parametri per le simulazioni che rimescolano i
// dati (shuffle). Tali parametri dipendono da: tipo di constraint, dati a
// disposizione e miglior topologia trovata nella prima fase Parametri di
// ingresso:	n_data = numero totale di dati a disposizione per
// l'apprendimento 				nn = numero di neuroni dello
// strato interno della miglior topologia trovata nella prima fase Parametro di
// uscita:		intero che identifica il numero di simulazioni da
// effettuare con lo shuffle
int init_shuffle_params(unsigned int n_data, unsigned int nn);

// Funzione che preprocessa il file contenente i dati di training per calcolare
// numero di dati, numero di ingressi e numero di modelli da apprendere. Questa
// funzione viene direttamente richiamata da "trainingfile_parser". Parametri di
// ingresso:	fd = file descriptor del file di training da preprocessare
// Parametro di uscita:		puntatore ad un array di 3 elementi contenente:
// in pos0 numero di dati totali a disposizione per l'apprendimento;
// in pos1
// numero di ingressi del modello; in pos2 numero di modelli.
unsigned int *preprocess_trainingfile(FILE *fd);

// Funzione che preprocessa il file contenente il dataset di soli ingressi di
// cui si vogliono calcolare le uscite. Questa funzione viene direttamente
// richiamata da "datafile_parser". Parametri di ingresso:	fd = file
// descriptor del file di ingressi da preprocessare Parametro di uscita:
// puntatore ad un array di 2 elementi contenente: in pos0 numero di dati
// contenuti nel dataset e di cui si vogliono calcolare
// le uscite; in pos1 numero di ingressi del modello.
unsigned int *preprocess_datafile(FILE *fd);

// Funzione che si occupa di parsificare il file di training che viene passato
// dall'utente tramite riga di comando. La parsificazione si occupa di
// convertire il file dal formato xdr al formato riconosciuto dalla libreria
// FANN. La funzione salva su disco dei file temporanei in un formato
// riconosciuto dalla libreria FANN. Tali file (chiamati model1_, model2_, ecc.)
// vengono eliminati dal disco subito dopo il completamento dell'apprendimento.
// Parametri di ingresso:	fin = stringa che identifica il nome del file in
// formato xdr da parsificare.
// Parametro di uscita:		intero che identifica il numero di modelli da
// apprendere.
unsigned int trainingfile_parser(char *fin);

// Funzione che si occupa di parsificare il file contenente gli ingressi da dare
// in pasto alla rete neurale. Questa funzione si occupa di allocare la memoria
// necessaria alla struttura Matrix che conterrà i risultati elaborati dalla
// rete neurale. Parametri di ingresso:	fin = stringa che identifica il nome del
// file in formato xdr da parsificare.
// Parametro di uscita:		struttura Matrix in cui inserire i risultati
// elaborati dalla rete neurale.
Matrix datafile_parser(char *fin);

// Funzione che si occupa di calcolare le uscite, utilizzando la rete neurale
// elaborata da "cascade_engine" e gli ingressi letti dal dataset. Parametri di
// ingresso:	networkfile = nome del file della rete neurale da utilizzare.
// Tale file viene salvato da "cascade_engine". mat = puntatore alla struttura
// dati inizializzata dal parsificatore, da cui leggere gli ingressi e in cui
// inserire le uscite calcolate dalla rete neurale. modello = modello di cui si
// vogliono calcolare
// le uscite. 				min = valore minimo calcolato da
//"compute_minmax" (per la denormalizzazione dei risultati)
// max = valore massimo calcolato da "compute_minmax" (per la denormalizzazione
// dei risultati)
// Parametro di uscita:		void.
void compute_outputdata(char *networkfile, Matrix *mat, unsigned short modello,
                        fann_type min, fann_type max);

// Funzione che si occupa di salvare su file la struttura dati Matrix
// contenente, nel momento in cui questa funzione viene richiamata, gli ingressi
// e le relative uscite di tutti i modelli. Parametri di ingresso:
// output_file = nome del file dal salvare (secondo la nostra convenzione il
// file verrà salvato come yyy_output dove yyy è
// il file del dataset di ingressi
// passato dall'utente tramite riga di comando). mat = struttura dati da salvare
// sul file. Parametro di uscita:		void.
void writefile(char *output_file, Matrix *mat);

#ifdef PRINT_REPORT
// Funzione utilizzata in "compute_shuffle", che si occupa di riempire il buffer
// del relativo thread con i messaggi di report forniti. In caso di overflow del
// buffer, la funzione si occupa del riallocamento di memoria necessario.
// Parametri di ingresso:	ingresso = puntatore alla struttura del thread
// che contiene il puntatore al buffer da riempire(si veda la struct shuffle_in)
//				stringline = array contenente la stringa da
// inserire nel buffer Parametro di uscita:		void.
void update_thread_buffer(shuffle_in *ingresso, char stringline[]);
#endif

// Funzione utilizzata per copiare la struttura della miglior rete trovata dalla
// funzione "compute_shuffle". Parametri di ingresso:	net = struttura della
// miglior rete da copiare. 				thread_num = numero
// identificativo del thread a cui
// associare la rete.
// Parametro di uscita:		struct fann* = puntatore alla struttura che
// identifica la copia della rete.
struct fann *my_fann_copy(struct fann *net, unsigned int thread_num);

// Procedura utilizzata per stampare a video un help relativo all'utilizzo del
// programma. Le istruzioni vengono visualizzate quando i parametri passati in
// ingresso sono errati, ovvero quando il programma non viene utilizzato
// correttamente.
void show_instruction();

// Funzione utilizzata per salvare su file i valori min e max delle uscite
// (letti dal file di training). Tali valori sono utili per denormalizzare i
// risultati quando il programma viene utilizzato per calcolare i risultati
// usando una rete precedentemente elaborata. Nota: Nella nostra
// implementazione, il nome del file contentente questi valori è il nome del
// file della rete seguito da _minmax. Esempio: se in nome della rete è model1,
// il file contenente il min e il max delle uscite di quel modello è
// model1_minmax Parametri di ingresso:	file = nome del file in cui memorizzare
// il minimo
// e il massimo. 				min = valore minimo.
// max = valore massimo. Parametro di uscita: void.
void save_minmax(char *file, fann_type min, fann_type max);

// Funzione utilizzata per caricare da file il minimo e massimo precedentemente
// salvati. Tali valori saranno utilizzati per denormalizzare i risultati
// calcolati dalla rete prima di scriverli su file.
// Parametri di ingresso:	file = nome del file da cui leggere il minimo e
// il
// massimo. 				min = puntatore in cui memorizzare il
// valore minimo letto dal file.
//				max = puntatore in cui memorizzare il valore
// massimo letto dal file. Parametro di uscita:		void.
void load_minmax(char *file, fann_type *min, fann_type *max);

// VARIABILI GLOBALI INIZIALIZZATE IN BASE AL CONSTRAINT
unsigned short MAX_NEURONS; // numero massimo di neuroni da aggiungere alla
                            // rete. Definisce l'upper bound per l'apprendimento
unsigned short RAY;         // raggio dell'intorno in cui esplorare
unsigned short E_Stop; // parametro relativo ai vari Early Stop implementati
unsigned short SHUFFLE_CONSTANT; // parametro utilizzato per determinare il
                                 // numero di simulazioni (simulation_value);

// VARIABILI GLOBALI
const char *constr = NULL; // memorizza il tipo di constraint passato
                           // dall'utente (vedi funzione init_params)
unsigned int num_dati_da_stimare =
    0; // memorizza il numero totale di dati da stimare (aggiornata da
       // "datafile_parser")
unsigned int model_number =
    1; // memorizza il numero totale di modelli da apprendere (aggiornata da
       // "trainingfile_parser")
int calcola_output = 1; // variabile booleana utilizzata come flag per calcolare
                        // o meno i risultati usando la rete

// VARIABILI GLOBALI CONDIVISE DAI THREAD
Matrix matrice; // matrice contenente gli ingressi e le uscite calcolate dalle
                // singole reti neurali
char *nome_modello; // stringa che identifica il nome del file di training. Ogni
                    // thread lavorerà sul proprio modello
char *network_filename; // stringa che identifica il file della rete neurale
char *output_filename;  // stringa che identifica il file in cui salvare i
                        // risultati elaborati dalla rete
char *program_name;     // stringa che identifica il nome del file del programma
unsigned int test_data_number; // numero di dati da utilizzare come test

#ifdef PRINT_REPORT
unsigned int
    buffer_size; // dimensione iniziale del buffer di ogni singolo THREAD
#endif

pthread_mutex_t mutexdata; // mutex utilizzato per eseguire atomicamente la
                           // funzione "my_fann_copy"

/*-----------------------------------------------------------------------------------------------------*/
/*					La funzione main
 */
/*-----------------------------------------------------------------------------------------------------*/

// il programma prende, attraverso riga di comando, il nome del file contenente
// i dati di training, il nome del file contenente il dataset di ingressi di cui
// si vogliono calcolare le uscite e il constraint.

// Esempio di utilizzo: ./cascade dataset_training.txt dataset_ingressi.txt fast
// | high | medium | low

int main(int argc, char *argv[]) {
  program_name = argv[0];
  if (argc == 1) {
    printf("\nError using program. Try %s --help for details. Aborting "
           "program...\n\n",
           program_name);
    exit(127);
  }

  int next_option;

  /* A string listing valid short options letters. */
  const char *const short_options = "ha:t:p:";

  /* An array describing valid long options. */
  const struct option long_options[] = {
      {"help", 0, NULL, 'h'},
      {"all", 1, NULL, 'a'},
      {"train", 1, NULL, 't'},
      {"predict", 1, NULL, 'p'},
      {NULL, 0, NULL, 0} /* Required at end of array. */
  };

  do {
    next_option = getopt_long(argc, argv, short_options, long_options, NULL);
    // int i;
    switch (next_option) {
    case 'h': /* -h or --help */
      show_instruction();
      exit(127);
    case 'a': /* -a or --all */
      if (argc < 7) {
        printf("\nMissing some arguments. Try %s --help for details. Aborting "
               "program...\n\n",
               argv[0]);
        exit(127);
      }
      nome_modello = argv[2];
      network_filename = argv[4];
      output_filename = argv[5];
      init_constr_params(argv[6]);
      model_number = trainingfile_parser(
          argv[2]); // generazione di m file di training nel formato FANN
      matrice = datafile_parser(argv[3]);
      printf("\nTotal model to analyse: %d\n\n", model_number);
      break;
    case 't': /* -t or --train */
      if (argc < 5) {
        printf("\nMissing some arguments. Try %s --help for details. Aborting "
               "program...\n\n",
               argv[0]);
        exit(127);
      }
      nome_modello = argv[2];
      init_constr_params(
          argv[4]); // settaggio dei parametri in base al constraint
      model_number = trainingfile_parser(
          argv[2]); // generazione di m file di training nel formato FANN
      network_filename = argv[3];
      printf("\nTotal model to analyse: %d\n\n", model_number);
      calcola_output = 0; // imposto a false la variabile, in questo modo non
                          // calcolo le uscite
      break;
    case 'p': /* -p or --predict */
      if (argc < 5) {
        printf("\nMissing some arguments. Try %s --help for details. Aborting "
               "program...\n\n",
               argv[0]);
        exit(127);
      }
      int i;
      unsigned int len;
      model_number = argc - 4; // numero di file di reti passati al programma
      fann_type min, max;
      matrice = datafile_parser(argv[2]);
      printf("\n");
      for (i = 0; i < model_number; i++) {
        len = strlen(argv[4 + i]); // i file delle reti neurali si trovano da
                                   // argv[4] in poi
        char str[len];
        strcpy(str, argv[4 + i]);
        char *file_minmax = strcat(str, "_minmax");
        // printf("\n");
        load_minmax(file_minmax, &min, &max);
        compute_outputdata(argv[4 + i], &matrice, i, min, max);
      }
      writefile(argv[3], &matrice);
      exit(127);
      break;
    case '?': /* The user specified an invalid option. */
      /* Print usage information to standard error, and exit with exit
      code one (indicating abnormal termination). */
      printf("\n Error using the program\n");
      show_instruction();
      exit(127);
    case -1: /* Done with options. */
      break;
    default: /* Something else: unexpected. */
      exit(127);
    }

  } while (next_option != -1);

  unsigned int i, j;
  int thread_creation_code;

  unsigned int bigforpar =
      model_number /
      MODEL_THREADS; // variabile che identifica quante volte eseguire i thread
                     // a gruppi di MODEL_THREADS
  unsigned int smallforpar =
      model_number %
      MODEL_THREADS; // variabile che identifica i thread rimanenti
                     // esempio se ci sono 23 modelli da stimare e MODEL_THREADS
                     // è impostato a 5, allora verranno elaborati per 4 volte 5
                     // modelli in parallelo (bigforpar = 4), ed infine verranno
                     // elaborati in parallelo i restanti 3 modelli (smallforpar
                     // = 3).

  pthread_mutex_init(&mutexdata, NULL);

  pthread_t cascade_thread[MODEL_THREADS];
  unsigned short cascade_in_par[MODEL_THREADS];

  for (i = 0; i < bigforpar; i++) {
    for (j = 0; j < MODEL_THREADS; j++) {
      cascade_in_par[j] = (i * MODEL_THREADS) + j;
      thread_creation_code = pthread_create(
          &cascade_thread[j], NULL, &cascade_engine, &cascade_in_par[j]);
      if (thread_creation_code) {
        printf("ERROR; return code from pthread_create() is %d\n",
               thread_creation_code);
        exit(-1);
      }
    }

    for (j = 0; j < MODEL_THREADS; j++) {
      pthread_join(cascade_thread[j], NULL);
    }
  }

  for (i = 0; i < smallforpar; i++) {
    cascade_in_par[i] = (bigforpar * MODEL_THREADS) + i;
    pthread_create(&cascade_thread[i], NULL, &cascade_engine,
                   &cascade_in_par[i]);
  }

  for (i = 0; i < smallforpar; i++) {
    pthread_join(cascade_thread[i], NULL);
  }

  pthread_mutex_destroy(&mutexdata);

  if (calcola_output)
    writefile(output_filename, &matrice);

  return 0;
}

/*-----------------------------------------------------------------------------------------------------*/
/*					Implementazione della FUNZIONE
 * PRINCIPALE del programma (cascade_engine)	       */
/*-----------------------------------------------------------------------------------------------------*/

// Funzione principale del programma che implementa la ricerca del miglior
// modello per le coppie ingresso/i-uscita/e fornite in ingresso Parametri di
// ingresso:	par = identifica il numero del thread, quindi il modello che
// quel particolare thread sta elaborando Parametro di uscita:		void. La
// funzione non ritorna nulla, ma salva su disco due file: "model"_bestnet.net
// contenente la miglior rete trovata e
// "model"_report.txt contenente un report sull'intero processo di
// apprendimento.
void *cascade_engine(void *par) {
  char data_file[40];
  unsigned short mod = *((unsigned short *)par);
  snprintf(data_file, sizeof(data_file), "model%d_", mod + 1);
  struct fann_train_data *all_data = fann_read_train_from_file(data_file);
  // printf("data_file:%s\n", data_file);
  remove(data_file);
  snprintf(data_file, sizeof(data_file), "%d", mod);
  // char *model = strcat(data_file, nome_modello); //esempio
  // model1_trainingfile
  unsigned short leng = strlen(network_filename);
  char net[leng];
  strcpy(net, network_filename);
  char *network_file_name = strcat(net, data_file); // esempio rete0, rete1,
                                                    // ecc.
  // printf("%s", network_file_name);
  // variabili per il calcolo del tempo di esecuzione
  time_t t1, t2;
  long int cpu_time_used;

  (void)time(&t1); // inizio del tempo di esecuzione
  time_t ltime = time(NULL);

  // variabili per la gestione del file di report e della miglior rete trovata
  unsigned short len = strlen(network_file_name);
  // char str[len];
  // strcpy(str, network_file_name);

#ifdef PRINT_REPORT
  char str[len];
  strcpy(str, network_file_name);
  // puntatore al file di report
  FILE *report_file;
  char *report_file_name = strcat(str, "_report.txt");
#endif

  // char *network_file_name = strcat(str, "_bestnet.net");
  // len = strlen(network_file_name);
  char str3[len];
  strcpy(str3, network_file_name);
  char *minmax_filename = strcat(str3, "_minmax");
  short count = 0; // Variabile di controllo (N.B.: questo contatore non va mai
                   // dichiarato "unsigned"...)
  unsigned short second_count = 0; // variabile di controllo
  unsigned short simulation_value; // variabile di controllo

  // NOTA: ai fini del nostro utilizzo, le seguenti variabili, da passare alla
  // funzione "fann_cascadetrain_on_data", possono essere fissate a questi
  // valori:
  const float desired_error = (const float)0;

#ifdef PRINT_STDOUT
  const unsigned int neurons_between_reports = 1;
#else
  printf("Training model %d with Cascade2 algorithm. This process could take "
         "several time, please wait....\n\n",
         mod + 1);
  const unsigned int neurons_between_reports = 0;
#endif

  // parametri per la seconda parte del programma (quella relativa allo SHUFFLE)
  // int max_neurons_to_add_shuffle;
  // int net_neurons_shuffle;
  // unsigned int nn_shuffle_best_net;
  unsigned int nn_next_shuffle_best_net;
  unsigned int nn_sum_shuffle_best_net;
  unsigned short count_div;
  float rest;

  fann_type e_test, e_train; // variabili utilizzate per memorizzare l'errore
                             // sui dati di test e sui dati di train
  fann_type e_test_best_net = 1e100; // teoricamente infinito
  unsigned int nn_best_net =
      0; // variabile utilizzata per memorizzare il numero di hidden layer della
         // topologia migliore finora trovata	(N.B.: è importante che sia
         // dichiarata "unsigned", per il fault tolerance)

  fann_type *f = compute_minmax(all_data);
  fann_type min = f[0];
  fann_type max = f[1];
  free(f);
  save_minmax(minmax_filename, min, max);
  // divido i dati: 20% per il testing e 80% per il training
  test_data_number = compute_test_data_number(fann_length_train_data(all_data),
                                              TEST_DATA_PERCENTAGE);

#ifdef PRINT_REPORT
  // apro il file di report in scrittura
  // NB. La directory di lavoro deve avere i permessi di scrittura
  report_file = fopen(report_file_name, "w");

  fprintf(report_file, "REPORT FILE MODEL %s\n\n", network_file_name);
  fprintf(report_file, "Training data: %d\n",
          (fann_length_train_data(all_data) - test_data_number));
  fprintf(report_file, "Testing data: %d\n", test_data_number);
  fprintf(report_file, "Total data: %d\n\n", fann_length_train_data(all_data));
  fprintf(report_file, "Model input number: %d\n",
          fann_num_input_train_data(all_data));
  fprintf(report_file, "Model output number: %d\n\n",
          fann_num_output_train_data(all_data));
  fprintf(report_file, "Hidden Layers\t Error on TESTING data (percent)\t "
                       "Error on TRAINING data (percent)\n");
#endif

  unsigned int controller =
      0; // variabile di controllo utilizzata nel ciclo while qui sotto
  float percent_accept =
      15; // percentuale che stabilisce "l'intorno di accettazione" dell'errore
          // sul testing, rispetto a quello sul training (per capire si veda da
          // Ln 181 a Ln 192)
  // unsigned short flag;		//variabile di controllo

  /*-----------------------------------------------------------------------------------------------------------*/
  /*PRIMA PARTE di cascade_engine, dove si ricerca la miglior topologia di rete
   * neurale con una prima scansione*/
  /*-----------------------------------------------------------------------------------------------------------*/

  while (controller <
         1) // while di controllo per la robustezza del programma; infatti con
            // pochi MAX_NEURONS si potrebbe anche non avere un minimo che
            // rientri nell'intorno di accettazione del 15%; quindi in tale
            // situazione, si aumenta la percentuale dell'intorno per cui
            // accettare il risultato trovato
  {
    controller++;

    // mescolo casualmente tutti i dati che ho a disposizione
    fann_shuffle_train_data(all_data);

    // divido i dati: 20% per il testing e 80% per il training
    struct fann_train_data *testing_data =
        fann_subset_train_data(all_data, 0, test_data_number);
    struct fann_train_data *training_data = fann_subset_train_data(
        all_data, test_data_number,
        fann_length_train_data(all_data) - test_data_number);

    // creo una rete neurale avente x neuroni di ingresso e y neuroni di uscita
    // (letti dai dati di training)
    struct fann *ann =
        fann_create_shortcut(2, fann_num_input_train_data(training_data),
                             fann_num_output_train_data(training_data));

    // inizializza i pesi usando l'algoritmo di Widrow + Nguyen
    fann_init_weights(ann, training_data);

    // imposto l'algoritmo di training
    fann_set_training_algorithm(ann, TRAINING_ALGORITHM);

    int i;
    unsigned int net_neurons = 0;
    unsigned int max_neurons_to_add = 1;

    for (i = 0; i < (MAX_NEURONS); i++) {
      net_neurons = net_neurons + 1; // aggiornamento del numero di neuroni
                                     // della topologia di rete attuale

      // normalizzo i dati di testing e di training tra 0 e 1	(questo perchè
      // vengono denormalizzati da calcola risultati, per questo ad ogni
      // iterazione del ciclo devo normalizzarli di nuovo)
      fann_scale_output_train_data(testing_data, 0, 1);
      fann_scale_output_train_data(training_data, 0, 1);

      // training della rete con vincolo di "nn" neuroni nell'hidden layer
      fann_cascadetrain_on_data(ann, training_data, max_neurons_to_add,
                                neurons_between_reports, desired_error);

      e_test = compute_error(ann, testing_data, min,
                             max); // calcolo dell'errore medio sul testing
      e_train = compute_error(ann, training_data, min,
                              max); // calcolo dell'errore medio sul training

#ifdef PRINT_STDOUT
      printf("With %d neurons:\n", net_neurons);
      printf(" The mean error on testing data is %f\n", e_test);
      printf(" The mean error on training data is %f\n\n", e_train);
#endif

#ifdef PRINT_REPORT
      // aggiorno il file di report con i risultati appena calcolati
      fprintf(report_file, "\t%d\t \t%f\t \t\t%f\n", net_neurons, e_test,
              e_train);
#endif

      /*parte per il salvataggio della miglior rete*/
      if (e_test < e_test_best_net) {
        if (e_test <= ((e_train) + ((e_train)*percent_accept /
                                    100))) // Upper bound per l'accettazione
        {
          if (e_test >=
              ((e_train) - ((e_train)*percent_accept / 100))) // Lower bound
          {
            e_test_best_net = e_test;
            nn_best_net = net_neurons;
            fann_save(
                ann,
                network_file_name); // salvataggio su file della miglior rete
          }
        }
      }

      /*parte che implementa i vari Early Stop*/

      /*Early Stop per overfitting*/
      if (e_test > ((e_train) + ((e_train)*20 / 100)))
        count = count + 1;
      else
        count = 0;

      /*Early Stop normale*/
      if (e_test > ((e_test_best_net) + ((e_test_best_net)*20 / 100)))
        second_count = second_count + 1;
      else
        second_count = 0;

      /*Early Stop per overfitting*/
      if (count >= E_Stop)
      // si interrompe la ricerca se l'errore medio sul testing si discosta di
      // più del 20% dall'errore medio sul training, per 5 o più passi
      // consecutivi
      {
#ifdef PRINT_STDOUT
        printf("\n Stop for overfitting\n");
#endif

#ifdef PRINT_REPORT
        fprintf(report_file, "\n    Stop for overfitting\n");
#endif
        break;
      }

      /*Early Stop normale*/
      if (second_count >= E_Stop)
      // si interrompe la ricerca se l'errore medio sul testing si discosta di
      // più del 20% dal miglior errore medio sul testing, per 5 o più passi
      // consecutivi
      {
#ifdef PRINT_STDOUT
        printf("\n Conditions for the Early Stop\n");
#endif

#ifdef PRINT_REPORT
        fprintf(report_file, "\n    Conditions for the Early Stop\n");
#endif
        break;
      }

    } // end for

    if (nn_best_net ==
        0) // contollore per la robustezza del programma; infatti con pochi
           // MAX_NEURONS si potrebbe anche non avere un minimo che rientri
           // nell'intorno di accettazione del 15%... Quindi, se si verifica
           // tale situazione si aumenta la percentuale di accettazione e si
           // reitera la prima parte del programma.
    {
      controller--; // per attivare la reiterazione della prima parte del
                    // programma
      percent_accept = ((percent_accept) +
                        (percent_accept * 20 /
                         100)); // incremento della percentuale di accettazione

#ifdef PRINT_STDOUT
      printf("\n Poor quality/quantity of the data provided to the program. "
             "Acceptance rate increased to %f percent\n\n",
             percent_accept);
#endif

#ifdef PRINT_REPORT
      fprintf(report_file,
              "\nPoor quality/quantity of the data provided to the program. "
              "Acceptance rate increased to %f percent\n\n",
              percent_accept);
#endif
    }

    // libero le risorse relative ai dati di training e di test
    fann_destroy_train(testing_data);
    fann_destroy_train(training_data);
    fann_destroy(ann);

  } // end while di controllo

#ifdef PRINT_STDOUT
  printf("\n The best topology for this first section is : %d neurons\n\n",
         nn_best_net);
#endif

#ifdef PRINT_REPORT
  fprintf(report_file,
          "\n    The best topology for this first section is : %d neurons\n",
          nn_best_net);
  fprintf(report_file, "\n-----------------------------------------------------"
                       "---------------------------------------------\n\n");
#endif
  /*---------------------------------------------------------------------------------------------------------*/
  /*SECONDA PARTE di cascade_engine, dove si rimescolano i dati (SHUFFLE) e si
   * "insegue" la miglior topologia*/
  /*---------------------------------------------------------------------------------------------------------*/

  if (strcmp(constr, "Fast")) // salto questa seconda parte della funzione se
                              // il constraint è impostato a "Fast"
  {

#ifdef PRINT_REPORT
    fprintf(report_file, "Report Info about researching around the best "
                         "network found with shuffling data\n\n");
    fprintf(report_file, "Hidden Layers\t Error on TESTING data (percent)\t "
                         "Error on TRAINING data (percent)\n");
#endif

    count = 0;
    count_div = 1;
    nn_sum_shuffle_best_net = nn_best_net;
    nn_next_shuffle_best_net = nn_best_net;

    // qui richiamo la funzione che inizializza i parametri per lo shuffle
    simulation_value =
        init_shuffle_params(fann_length_train_data(all_data), nn_best_net);
    pthread_t shuffle_thread[SHUFFLE_THREADS];
    shuffle_in in_par[SHUFFLE_THREADS];

    unsigned int t;
#ifdef PRINT_REPORT
    buffer_size =
        ((((50 * ((2 * (RAY)) + 1)) + 100) + 1000) *
         100); // dimensione iniziale del buffer di ogni singolo THREAD; la
               // dimensione iniziale di tale buffer è frutto di una stima che
               // considera : RAY=raggio dell'intorno da esplorare;
               // 50=dimensione di ogni riga da stampare; 100=dimensione
               // dell'ultima scritta da stampare; 1000=margine per tre
               // eventuali proseguimenti; quest'ultimi 3 valori sono stati
               // approssimati per eccesso. L'ultimo fattore indica invece il
               // numero delle sessioni. Quindi per riassumere, inizialmente per
               // il buffer di ogni thread viene allocata memoria per circa le
               // prime 100 sessioni; se eventualmente si necessiterà di
               // ulteriore memoria questa verrà riallocata (si veda la funzione
               // "update_thread_buffer").
#endif

    // INIZIALIZZAZIONE DELLE VARIABILI DI INGRESSO DA DARE AL THREAD
    for (t = 0; t < SHUFFLE_THREADS; t++) {
      in_par[t].all_data = fann_duplicate_train_data(all_data);
      in_par[t].min = min;
      in_par[t].max = max;
      in_par[t].thread_number = t;

      in_par[t].best_net_shuffle = NULL;

#ifdef PRINT_REPORT
      in_par[t].buffer_session = calloc(buffer_size, sizeof(char));
      in_par[t].lunghezza_tot = 0;
      in_par[t].dimensione_buffer = buffer_size;
#endif
    }
    while (count < simulation_value) {
      int rc;

      for (t = 0; t < SHUFFLE_THREADS; t++) {

        in_par[t].nn_next_shuffle_best_net =
            nn_next_shuffle_best_net; // aggiornamento della topologia ideale,
                                      // che è il centro dell'intorno di
                                      // esplorazione...
        rc = pthread_create(&shuffle_thread[t], NULL, &compute_shuffle,
                            &in_par[t]);
        if (rc) {
          printf("ERROR; return code from pthread_create() is %d\n", rc);
          exit(-1);
        }
      }

      for (t = 0; t < SHUFFLE_THREADS; t++) {
        pthread_join(shuffle_thread[t], NULL);
      }

      unsigned short flag_decrement = 0;
#ifdef PRINT_REPORT
      char terminal_line[200];
#endif
      fann_type e_test_best_net_shuffle;
      unsigned int nn_shuffle_best_net;

      for (t = 0; t < SHUFFLE_THREADS; t++) {
        e_test_best_net_shuffle = in_par[t].e_test_best_net_shuffle;
        nn_shuffle_best_net = in_par[t].nn_shuffle_best_net;

        /*parte per il salvataggio della miglior rete*/
        if (e_test_best_net_shuffle < e_test_best_net) {
          e_test_best_net = e_test_best_net_shuffle;
          nn_best_net = nn_shuffle_best_net;
          fann_save(in_par[t].best_net_shuffle, network_file_name);

          if (flag_decrement == 0) {
            count = -1;
            flag_decrement =
                1; // si decrementa solo una volta, indipendentemente dal numero
                   // di shuffles paralleli eseguiti
          }
        }

        // si verifica se considerare l'attuale section, per il calcolo della
        // topologia "ideale"
        if (e_test_best_net_shuffle <=
            ((e_test_best_net) + ((e_test_best_net)*70 / 100))) {
#ifdef PRINT_STDOUT
          printf("\n THREAD NUMBER %d , The best topology for this section is "
                 ": %d neurons\n",
                 in_par[t].thread_number, nn_shuffle_best_net);
#endif

#ifdef PRINT_REPORT
          sprintf(terminal_line,
                  "\n    The best topology for this section is : %d neurons\n",
                  nn_shuffle_best_net);
          update_thread_buffer(&in_par[t], terminal_line);
#endif

          count++;
          count_div++;

          // calcolo della miglior topologia "ideale", calcolata come la media
          // delle migliori topologie trovate nelle singole iterazioni
          // precedenti
          nn_sum_shuffle_best_net =
              (nn_sum_shuffle_best_net + nn_shuffle_best_net);
          nn_next_shuffle_best_net = ((nn_sum_shuffle_best_net) / (count_div));
          float modulo = ((nn_sum_shuffle_best_net) % (count_div));
          rest = (modulo / count_div);
          if (rest > 0.5) {
            nn_next_shuffle_best_net++; // arrotondamento per eccesso se il
                                        // resto è > 0.5
          }
#ifdef PRINT_STDOUT
          printf("\n At the moment the best 'ideal' network topology is : %d "
                 "neurons\n\n",
                 nn_next_shuffle_best_net);
#endif

#ifdef PRINT_REPORT
          sprintf(terminal_line,
                  "\n    At the moment the best 'ideal' network topology is : "
                  "%d neurons\n\n",
                  nn_next_shuffle_best_net);
          update_thread_buffer(&in_par[t], terminal_line);
          sprintf(terminal_line, "\n    "
                                 "---------------------------------------------"
                                 "---------------------\n\n");
          update_thread_buffer(&in_par[t], terminal_line);
#endif
        }

        else {
#ifdef PRINT_STDOUT
          printf("\n THREAD NUMBER %d , Really bad performances, this section "
                 "does not contribute to the best 'ideal' network topology "
                 "estimate\n\n",
                 in_par[t].thread_number);
#endif

#ifdef PRINT_REPORT
          sprintf(
              terminal_line,
              "\n    Really bad performances, this section does not contribute "
              "to the best 'ideal' network topology estimate\n\n");
          update_thread_buffer(&in_par[t], terminal_line);
          sprintf(terminal_line, "\n    "
                                 "---------------------------------------------"
                                 "---------------------\n\n");
          update_thread_buffer(&in_par[t], terminal_line);
#endif
        }
      } // end for
    }   // end while

#ifdef PRINT_REPORT
      // Stampa delle sessioni su file
    for (t = 0; t < SHUFFLE_THREADS; t++) {
      fprintf(report_file, "\nTHREAD NUMBER %d's REPORT:\n\n", t);
      fprintf(report_file, "%s", in_par[t].buffer_session);
    }
#endif
  } // end if

  (void)time(&t2); // fine del tempo di esecuzione
  cpu_time_used = (difftime(t2, t1)) / 60;

#ifdef PRINT_REPORT
  fprintf(report_file, "\n-----------------------------------------------------"
                       "---------------------------------------------\n\n");
  fprintf(report_file, "FINAL REPORT\n\n");
  fprintf(report_file, "Constraint type: %s \n", constr);
  fprintf(report_file, "Best Network Topology: %d Hidden Layers\n",
          nn_best_net);
  fprintf(report_file, "Best testing data error: %f percent\n",
          e_test_best_net);
  fprintf(report_file, "Neural Network File Object: %s\n\n", network_file_name);
  fprintf(report_file, "Total running time: %d minutes\n\n\n", cpu_time_used);
  fprintf(report_file, "Date File Creation: %s\n\n", ctime(&ltime));
  fclose(report_file);
#endif

#ifdef PRINT_STDOUT
  // printf di output
  printf("\n\n");
  printf("The best average error on the test data is %f \n", e_test_best_net);
  printf("Neuron number of the best network: %d \n", nn_best_net);
  printf("Total running time: %ld minutes\n\n\n", cpu_time_used);
#endif

  printf("Training model %d completed. ", mod + 1);
  if (!calcola_output)
    printf("\n\n");
  fann_destroy_train(all_data);
  if (calcola_output)
    compute_outputdata(network_file_name, &matrice, mod, min, max);
  return NULL;
} // end cascade_engine function

/*-----------------------------------------------------------------------------------------------*/
/*			Implementazione delle funzioni di utility utilizzate
 */
/*-----------------------------------------------------------------------------------------------*/

// Funzione che calcola i valori minimo e massimo delle uscite del dataset
// passato in ingresso
// Parametri di ingresso:	dati = dataset di cui si vogliono calcolare i
// valori minimo e massimo delle uscite
// Parametro di uscita:		puntatore ad un array di due elementi con il
// valore min in pos 0 e il valore massimo in pos 1
fann_type *compute_minmax(struct fann_train_data *dati) {
  unsigned int i, j; // per la gestione dei cicli

  fann_type old_min, old_max, temp;

  fann_type *minmax = (fann_type *)malloc(2 * sizeof(fann_type));
  fann_type **uscite = dati->output; // lettura dell'array di output
  unsigned int numero_dati =
      dati->num_data; // lettura del numero di dati a disposizione
  unsigned int numero_uscite =
      dati->num_output; // lettura del numero di output per ogni pattern di dati
                        // (generalmente 1)

  old_min = old_max = uscite[0][0];

  for (i = 0; i < numero_dati; i++) {
    for (j = 0; j < numero_uscite; j++) {
      temp = uscite[i][j];
      if (temp < old_min)
        old_min = temp;
      else if (temp > old_max)
        old_max = temp;
    }
  }

  minmax[0] = old_min;
  minmax[1] = old_max;

  return minmax;
}

// Funzione che calcola l'errore percentuale medio utilizzando la rete calcolata
// da cascade2 Parametri di ingresso:	ann = struttura della rete neurale da
// utilizzare per il calcolo dell'errore testing_data = dataset su cui calcolare
// l'errore 				min, max = valori minimo e massimo
// utilizzati per la denormalizzazione Parametro di uscita:		errore
// percentuale medio pesato
fann_type compute_error(struct fann *ann, struct fann_train_data *testing_data,
                        fann_type min, fann_type max) {
  unsigned int i;
  fann_type *ris;
  fann_type somma, temp;
  somma = 0;
  struct fann_train_data *testing_data_calcolati =
      fann_duplicate_train_data(testing_data);

  fann_type **ingressi = testing_data_calcolati->input;

  unsigned int numero_dati =
      testing_data_calcolati
          ->num_data; // lettura del numero di dati a disposizione

  for (i = 0; i < numero_dati; i++) {
    ris = fann_run(ann, ingressi[i]);
    testing_data_calcolati->output[i][0] =
        ris[0]; // pensato per modelli aventi una sola uscita...
  }
  fann_scale_output_train_data(testing_data_calcolati, min, max);
  fann_scale_output_train_data(testing_data, min, max);
  for (i = 0; i < numero_dati; i++) {
    temp = fabs(
        ((testing_data_calcolati->output[i][0] - testing_data->output[i][0]) *
         100) /
        testing_data->output[i][0]);

    somma += temp;
  }

  fann_type media = somma / numero_dati;

  return media;
}

// Funzione che calcola il numero di dati di test da utilizzare come percentuale
// sui dati totali (es #test = 20% dei dati tot) Parametri di ingresso:
// total_data_number = numero di dati totali a disposizione per l'apprendimento
//				percent_value = valore percentuale
// Parametro di uscita:		numero di dati di test
unsigned int compute_test_data_number(unsigned int total_data_number,
                                      unsigned short percent_value) {
  return (floor((total_data_number)*percent_value / 100));
}

// Funzione che inizializza i parametri di apprendimento in base al tipo di
// constraint passato dall'utente Parametri di ingresso:	constraint =
// tipo di constraint da utilizzare per l'apprendimento (scelto tra LOW, MEDIUM,
// HIGH, FAST) Parametro di uscita:		void.
void init_constr_params(char *constraint) {
  // if(constraint==NULL) constraint="High"; //se non viene inserito nessun tipo
  // di constraint, di default viene considerato "High"

  switch (tolower(constraint[0])) {
  case 'f':
    printf("\nConstraint chosen: FAST\n\n");
    constr = "Fast";
    MAX_NEURONS = 50;
    E_Stop = 5;
    break;

  case 'h':
    printf("\nConstraint chosen: HIGH\n\n");
    constr = "High";
    MAX_NEURONS = 50;
    RAY = 8;
    E_Stop = 5;
    SHUFFLE_CONSTANT = 1;
    break;

  case 'l':
    printf("\nConstraint chosen: LOW\n\n");
    constr = "Low";
    MAX_NEURONS = 300;
    RAY = 15;
    E_Stop = 10;
    SHUFFLE_CONSTANT = 10;
    break;

  default:
    printf("\nConstraint chosen: MEDIUM\n\n");
    constr = "Medium";
    MAX_NEURONS = 100;
    RAY = 10;
    E_Stop = 7;
    SHUFFLE_CONSTANT = 3;
    break;
  }
}

// Funzione che determina il numero di iterazioni ("n_shuffle") della seconda
// parte del programma (quella relativa allo SHUFFLE) in base al tipo di
// constraint, al numero di dati forniti al programma e al numero di neuroni
// della miglior topologia di rete trovata nella prima parte della funzione
// "cascade_engine". Funzione che inizializza i parametri per le simulazioni che
// rimescolano i dati (shuffle). Tali parametri dipendono da: tipo di
// constraint, dati a disposizione e miglior topologia trovata nella prima fase
// Parametri di ingresso:	n_data = numero totale di dati a disposizione
// per
// l'apprendimento 				nn = numero di neuroni dello
// strato interno della miglior topologia trovata nella prima fase Parametro di
// uscita:		intero che identifica il numero di simulazioni da
// effettuare con lo shuffle
int init_shuffle_params(unsigned int n_data, unsigned int nn) {
  int n_shuffle;

  if (n_data <= 1000) {
    if (nn <= 10)
      n_shuffle = 15 * SHUFFLE_CONSTANT;
    if (nn > 10 && nn <= 15)
      n_shuffle = 10 * SHUFFLE_CONSTANT;
    if (nn > 15 && nn <= 30)
      n_shuffle = 8 * SHUFFLE_CONSTANT;
    if (nn > 30 && nn <= 50)
      n_shuffle = 6 * SHUFFLE_CONSTANT;
    if (nn > 50)
      n_shuffle = 4 * SHUFFLE_CONSTANT;
  }
  if (n_data > 1000 && n_data <= 2000) {
    if (nn <= 10)
      n_shuffle = 8 * SHUFFLE_CONSTANT;
    if (nn > 10 && nn <= 15)
      n_shuffle = 6 * SHUFFLE_CONSTANT;
    if (nn > 15 && nn <= 30)
      n_shuffle = 4 * SHUFFLE_CONSTANT;
    if (nn > 30 && nn <= 50)
      n_shuffle = 3 * SHUFFLE_CONSTANT;
    if (nn > 50)
      n_shuffle = 2 * SHUFFLE_CONSTANT;
  }
  if (n_data > 2000 && n_data <= 4000) {
    if (nn <= 10)
      n_shuffle = 7 * SHUFFLE_CONSTANT;
    if (nn > 10 && nn <= 15)
      n_shuffle = 5 * SHUFFLE_CONSTANT;
    if (nn > 15 && nn <= 25)
      n_shuffle = 3 * SHUFFLE_CONSTANT;
    if (nn > 25)
      n_shuffle = 2 * SHUFFLE_CONSTANT;
  }
  if (n_data > 4000 && n_data <= 6000) {
    if (nn <= 10)
      n_shuffle = 5 * SHUFFLE_CONSTANT;
    if (nn > 10 && nn <= 15)
      n_shuffle = 3 * SHUFFLE_CONSTANT;
    if (nn > 15 && nn <= 25)
      n_shuffle = 2 * SHUFFLE_CONSTANT;
    if (nn > 25)
      n_shuffle = 1 * SHUFFLE_CONSTANT;
  }
  if (n_data > 6000 && n_data <= 10000) {
    if (nn <= 10)
      n_shuffle = 3 * SHUFFLE_CONSTANT;
    if (nn > 10 && nn <= 15)
      n_shuffle = 2 * SHUFFLE_CONSTANT;
    if (nn > 15)
      n_shuffle = 1 * SHUFFLE_CONSTANT;
  }
  if (n_data > 10000) {
    // caso con constraint LOW
    if (!strcmp(constr, "High"))
      n_shuffle = 10;
    // caso con constraint diverso da LOW
    if ((strcmp(constr, "High")))
      n_shuffle = 1;
  }

  return n_shuffle;
}

// Funzione che preprocessa il file contenente i dati di training per calcolare
// numero di dati, numero di ingressi e numero di modelli da apprendere. Questa
// funzione viene direttamente richiamata da "trainingfile_parser". Parametri di
// ingresso:	fd = file descriptor del file di training da preprocessare
// Parametro di uscita:		puntatore ad un array di 3 elementi contenente:
// in pos0 numero di dati totali a disposizione per l'apprendimento;
// in pos1
// numero di ingressi del modello; in pos2 numero di modelli.
unsigned int *preprocess_trainingfile(FILE *fd) {
  double tmp;
  char ch;
  unsigned int i;
  unsigned int *datainfo = (unsigned int *)malloc(3 * sizeof(unsigned int));
  printf("Start prepocessing training data file.....");
  while (1) {
    fscanf(fd, "%d", &datainfo[1]);

    for (i = 0; i < datainfo[1]; i++)
      fscanf(fd, "%lf", &tmp);

    fscanf(fd, "%d", &datainfo[2]);
    if (datainfo[2])
      break;
  }

  datainfo[0] = 0;
  rewind(fd);
  while (fscanf(fd, "%c", &ch) > 0)
    if (ch == '\n')
      datainfo[0]++;
  rewind(fd);
  printf("Done\n");
  return datainfo;
}

// Funzione che preprocessa il file contenente il dataset di soli ingressi di
// cui si vogliono calcolare le uscite. Questa funzione viene direttamente
// richiamata da "datafile_parser". Parametri di ingresso:	fd = file
// descriptor del file di ingressi da preprocessare Parametro di uscita:
// puntatore ad un array di 2 elementi contenente: in pos0 numero di dati
// contenuti nel dataset e di cui si vogliono calcolare
// le uscite; in pos1 numero di ingressi del modello.
unsigned int *preprocess_datafile(FILE *fd) {

  char ch;
  // double tmp;
  // unsigned int i;
  unsigned int *datainfo = (unsigned int *)malloc(2 * sizeof(unsigned int));

  printf("Start prepocessing data file.....");

  fscanf(fd, "%d", &datainfo[1]);
  /*for(i=0; i<datainfo[1]; i++)
  {
          fscanf(fd, "%lf", &tmp);
  }
  fscanf(fd, "%d", &datainfo[2]);*/
  datainfo[0] = 0;
  rewind(fd);
  while (fscanf(fd, "%c", &ch) > 0) {
    if (ch == '\n') {
      datainfo[0]++;
    }
  }

  rewind(fd);
  printf("Done\n");
  return datainfo;
}

// Funzione che si occupa di parsificare il file contenente gli ingressi da dare
// in pasto alla rete neurale. Questa funzione si occupa di allocare la memoria
// necessaria alla struttura Matrix che conterrà i risultati elaborati dalla
// rete neurale. Parametri di ingresso:	fin = stringa che identifica il nome del
// file in formato xdr da parsificare.
// Parametro di uscita:		struttura Matrix in cui inserire i risultati
// elaborati dalla rete neurale.
Matrix datafile_parser(char *fin) {
  unsigned int i, j;
  char ch;
  fann_type temp_data;
  Matrix mat;
  FILE *dati_da_elaborare =
      fopen(fin, "r"); // apertura del file contente i dati da calcolare

  if (dati_da_elaborare == NULL) {
    /* L'apertura del file è fallita. Stampo un messaggio di errore ed esco dal
     * programma */
    printf("\nError opening input file: %s. Aborting program...\n\n", fin);
    // rimozione dei file temporanei creati dal parsificatore dei dati di
    // training
    char data_file[40];
    int i = 0;
    for (i = 0; i < model_number; i++) {
      snprintf(data_file, sizeof(data_file), "model%d_", i + 1);
      remove(data_file);
    }
    exit(1);
  }

  unsigned int *d = preprocess_datafile(dati_da_elaborare);
  unsigned int data_number = num_dati_da_stimare = d[0];
  unsigned int input_number =
      d[1] + 2; // inserimento di ulteriori due colonne per memorizzare num di
                // ingressi e num di uscite

  mat.input = (fann_type **)calloc(input_number, sizeof(fann_type *));
  mat.output = (fann_type **)calloc(model_number, sizeof(fann_type *));
  for (i = 0; i < input_number; i++) {
    mat.input[i] = (fann_type *)calloc(data_number, sizeof(fann_type));
  }
  for (i = 0; i < model_number; i++) {
    mat.output[i] = (fann_type *)calloc(data_number, sizeof(fann_type));
  }

  for (i = 0; i < data_number; i++) {
    for (j = 0; j < (input_number - 1); j++) {
      fscanf(dati_da_elaborare, "%f", &temp_data);
      mat.input[j][i] = temp_data;
    }

    mat.input[j][i] = model_number;

    while (1) {
      fscanf(dati_da_elaborare, "%c", &ch);
      if (ch == '\n')
        break;
    }
  }

  fclose(dati_da_elaborare);

  return mat;
}

// Funzione che si occupa di parsificare il file di training che viene passato
// dall'utente tramite riga di comando. La parsificazione si occupa di
// convertire il file dal formato xdr al formato riconosciuto dalla libreria
// FANN. La funzione salva su disco dei file temporanei in un formato
// riconosciuto dalla libreria FANN. Tali file (chiamati model1_, model2_, ecc.)
// vengono eliminati dal disco subito dopo il completamento dell'apprendimento.
// Parametri di ingresso:	fin = stringa che identifica il nome del file in
// formato xdr da parsificare.
// Parametro di uscita:		intero che identifica il numero di modelli da
// apprendere.
unsigned int trainingfile_parser(char *fin) {
  FILE *file_ingresso; // descrittore del file di ingresso
  // variabili di ciclo
  unsigned short i = 0;
  unsigned short j = 0;
  unsigned int count;

  file_ingresso = fopen(fin, "r"); // apertura del file da cui leggere i dati

  if (file_ingresso == NULL) {
    /* L'apertura del file è fallita. Stampo un messaggio di errore ed esco dal
     * programma */
    printf("Error opening training file: %s. Aborting program...\n\n", fin);
    exit(1);
  }

  unsigned int *d = preprocess_trainingfile(
      file_ingresso); // si preprocessa il file di ingresso per calcolare numero
                      // dati, ingressi e uscite
  unsigned int data_number = d[0];
  unsigned int input_number = d[1];
  unsigned int output_number = d[2];
  printf("Converting training data file.....");
  free(d);
  char file[40]; // buffer utilizzato per memorizzare l'intero convertito in
                 // stringa; vedi funzione snprintf

  // creazione di un file di output in formato FANN per ogni modello (quindi un
  // file per ogni uscita)
  FILE *output_files[output_number];
  for (i = 0; i < output_number; i++) {
    snprintf(file, sizeof(file), "model%d_",
             i + 1); // per convertire un intero in stringa
    output_files[i] = fopen(file, "w");
    fprintf(output_files[i], "%d %d 1\n", data_number, input_number);
  }

  int temp; // variabile utilizzata per memorizzare temporaneamente il dato
            // letto dal file di ingresso(swap)
  double ingressi[input_number];
  double out;
  unsigned int data_with_zero_output = 0;
  rewind(file_ingresso); // ci si posiziona all'inizio del file di input per
                         // poter cominciare la parsificazione

  // lettura del file di input e scrittura dei dati negli "output_number" files
  // (formato FANN)
  for (count = 0; count < data_number; count++) {
    fscanf(file_ingresso, "%d",
           &temp); // lettura del primo numero ovvero quanti ingressi ci sono:
                   // dato che butto via perchè ridondante

    for (j = 0; j < input_number; j++) {
      fscanf(file_ingresso, "%lf", &ingressi[j]);
    }

    fscanf(file_ingresso, "%d", &temp); // lettura del numero di uscite
    if (temp) // se temp !0 (vuol dire che ci sono delle uscite nella riga
              // appena letta) si salvano i dati della riga letta negli
              // outputfiles
    {
      for (i = 0; i < output_number;
           i++) // scrittura dei dati di ingresso letti dal file sorgente per
                // ogni file di output (uno per ogni modello)
      {
        for (j = 0; j < input_number; j++)
          fprintf(output_files[i], "%lf ", ingressi[j]);
        fprintf(output_files[i], "\n");
        fscanf(file_ingresso, "%lf", &out); // lettura dell'uscita i-esima
        fprintf(output_files[i], "%lf\n",
                out); // scrittura dell'uscita i-esima sull'i-esimo file
      }
    } else {
      data_with_zero_output++;
    }
  }
  // chiusura di tutti i file utilizzati
  fclose(file_ingresso);
  for (i = 0; i < output_number; i++) {
    // se ci sono dati con zero output, questi non vengono considerati, quindi
    // si aggiorna il numero di dati totali nei file di output
    if (data_with_zero_output) {
      rewind(output_files[i]);
      fprintf(output_files[i], "%d", (data_number - data_with_zero_output));
    }
    fclose(output_files[i]);
  }
  printf("Done\n");
  return output_number;
}

// Funzione che si occupa di calcolare le uscite, utilizzando la rete neurale
// elaborata da "cascade_engine" e gli ingressi letti dal dataset. Parametri di
// ingresso:	networkfile = nome del file della rete neurale da utilizzare.
// Tale file viene salvato da "cascade_engine". mat = puntatore alla struttura
// dati inizializzata dal parsificatore, da cui leggere gli ingressi e in cui
// inserire le uscite calcolate dalla rete neurale. modello = modello di cui si
// vogliono calcolare
// le uscite. 				min = valore minimo calcolato da
//"compute_minmax" (per la denormalizzazione dei risultati)
// max = valore massimo calcolato da "compute_minmax" (per la denormalizzazione
// dei risultati)
// Parametro di uscita:		void.
void compute_outputdata(char *networkfile, Matrix *mat, unsigned short modello,
                        fann_type min, fann_type max) {
  unsigned int count, i;

  unsigned int data_number = num_dati_da_stimare;

  unsigned int num_input = (int)mat->input[0][0];

  fann_type ingressi[num_input];
  struct fann *ann = fann_create_from_file(networkfile);

  if (ann == NULL) {
    printf("\nError opening neural network file: %s. Aborting program...\n",
           networkfile);
    exit(1);
  }
  printf("Computing output data.....");
  // lettura dell'input dal file dei dati
  for (count = 0; count < data_number; count++) {
    for (i = 0; i < num_input; i++)
      ingressi[i] = mat->input[i + 1][count];
    fann_type *ris = fann_run(ann, ingressi);
    mat->output[modello][count] = (ris[0] * (max - min) + min);
  }

  fann_destroy(ann);
  printf("Done\n\n");
}

// Funzione che si occupa di salvare su file la struttura dati Matrix
// contenente, nel momento in cui questa funzione viene richiamata, gli ingressi
// e le relative uscite di tutti i modelli. Parametri di ingresso:
// output_file = nome del file dal salvare (secondo la nostra convenzione il
// file verrà salvato come yyy_output dove yyy è
// il file del dataset di ingressi
// passato dall'utente tramite riga di comando). mat = struttura dati da salvare
// sul file. Parametro di uscita:		void.
void writefile(char *output_file, Matrix *mat) {
  // char *final_output_file = strcat(output_file, "_output");
  printf("Writing results on output file.....");
  unsigned int i, j;
  unsigned int num_input = (unsigned int)mat->input[0][0];

  FILE *fd = fopen(output_file, "w");
  for (i = 0; i < num_dati_da_stimare; i++) {
    for (j = 0; j < (num_input + 2); j++)
      fprintf(fd, "%d ", (int)mat->input[j][i]);

    for (j = 0; j < model_number; j++)
      fprintf(fd, "%lg ", mat->output[j][i]);
    fprintf(fd, "\n");
  }
  fclose(fd);
  printf("Done\n\n");
}

void *compute_shuffle(void *par) {
#ifdef PRINT_REPORT
  char line[200]; // array temporaneo di 200 caratteri in cui inserire le righe
                  // di ogni stampa
#endif

  shuffle_in *ingresso = ((shuffle_in *)par);

  ingresso->e_test_best_net_shuffle = 1e100; // teoricamente infinito

  int net_neurons_shuffle;
  int max_neurons_to_add_shuffle;

#ifdef PRINT_STDOUT
  const unsigned int neurons_between_reports = 1;
#else
  const unsigned int neurons_between_reports = 0;
#endif

  const float desired_error = (const float)0;
  fann_type e_test, e_train; // variabili utilizzate per memorizzare l'errore
                             // sui dati di test e sui dati di train

  fann_shuffle_train_data(
      ingresso->all_data); // si mescolano casualmente tutti i dati che si hanno
                           // a disposizione

  struct fann_train_data *testing_data =
      fann_subset_train_data(ingresso->all_data, 0, test_data_number);
  struct fann_train_data *training_data = fann_subset_train_data(
      ingresso->all_data, test_data_number,
      fann_length_train_data(ingresso->all_data) - test_data_number);

  // creazione di una rete neurale avente x neuroni di ingresso e y neuroni di
  // uscita (letti dai dati di training)
  struct fann *ann =
      fann_create_shortcut(2, fann_num_input_train_data(training_data),
                           fann_num_output_train_data(training_data));

  fann_init_weights(ann, training_data); // inizializzazione dei pesi usando
                                         // l'algoritmo di Widrow + Nguyen
  fann_set_training_algorithm(
      ann, TRAINING_ALGORITHM); // impostazione dell'algoritmo di training

  unsigned int y;

  unsigned short count_shuffle = 0;
  unsigned short for_condition = (((RAY)*2) + 1);
  unsigned short count_for_iteration = 0;
  ingresso->nn_shuffle_best_net = ingresso->nn_next_shuffle_best_net;
  net_neurons_shuffle = (ingresso->nn_shuffle_best_net - RAY);
  max_neurons_to_add_shuffle = (ingresso->nn_shuffle_best_net - RAY);

  for (y = 1; y <= for_condition; y++) {
    count_for_iteration++;

    /*per evitare possibili casi di “max_neurons_to_add” negativi o uguali a
     * zero (molto improbabili ma non impossibili…)*/
    if (max_neurons_to_add_shuffle <= 0) {
      max_neurons_to_add_shuffle = 1;
      net_neurons_shuffle = 1;
    }

    // normalizzazione dei dati di testing e di training tra 0 e 1
    fann_scale_output_train_data(testing_data, 0, 1);
    fann_scale_output_train_data(training_data, 0, 1);

    // training della rete con vincolo di "nn" neuroni nell'hidden layer
    fann_cascadetrain_on_data(ann, training_data, max_neurons_to_add_shuffle,
                              neurons_between_reports, desired_error);
    e_test = compute_error(ann, testing_data, ingresso->min, ingresso->max);
    e_train = compute_error(ann, training_data, ingresso->min, ingresso->max);
#ifdef PRINT_STDOUT
    printf(" THREAD NUMBER %d , With %d neurons:\n", ingresso->thread_number,
           net_neurons_shuffle);
    printf(" The mean error on testing data is %f\n", e_test);
    printf(" The mean error on training data is %f\n\n", e_train);
#endif

#ifdef PRINT_REPORT
    // aggiornamento del file di report con i risultati appena calcolati
    sprintf(line, "\t%d\t \t%f\t \t\t%f\n", net_neurons_shuffle, e_test,
            e_train);
    update_thread_buffer(ingresso, line);
#endif
    /*parte per il salvataggio della miglior topologia locale per
     * l'implementazione dell'inseguimento*/
    if (e_test < ingresso->e_test_best_net_shuffle) {
      if (e_test <=
          ((e_train) + ((e_train)*15 / 100))) // Upper bound di accettazione
      {
        if (e_test >=
            ((e_train) - ((e_train)*15 / 100))) // Lower bound di accettazione
        {
          ingresso->e_test_best_net_shuffle = e_test;
          ingresso->nn_shuffle_best_net = net_neurons_shuffle;
          pthread_mutex_lock(&mutexdata); // lock mutex
          ingresso->best_net_shuffle =
              my_fann_copy(ann, ingresso->thread_number);
          pthread_mutex_unlock(&mutexdata); // unlock mutex
        }
      }
    }

    net_neurons_shuffle = net_neurons_shuffle + 1;
    max_neurons_to_add_shuffle = 1;

    /*Early Stop per overfitting*/
    if (e_test > ((e_train) + ((e_train)*15 / 100)))
      count_shuffle = count_shuffle + 1;
    else
      count_shuffle = 0;
    if (count_shuffle >= E_Stop) {
#ifdef PRINT_STDOUT
      printf("\n THREAD NUMBER %d , Stop this section for overfitting\n",
             ingresso->thread_number);
#endif

#ifdef PRINT_REPORT
      sprintf(line, "\n    Stop this section for overfitting\n");
      update_thread_buffer(ingresso, line);
#endif
      break;
    }

    // si verifica se sussistono le condizioni per il proseguimento dell'attuale
    // ricerca
    if ((count_for_iteration == for_condition &&
         e_test <= ((ingresso->e_test_best_net_shuffle) +
                    ((ingresso->e_test_best_net_shuffle) * 5 / 100)) &&
         e_test <= ((e_train) + ((e_train)*10 / 100)) &&
         e_test >= ((e_train) - ((e_train)*15 / 100)))) {
      for_condition =
          for_condition +
          5; // si fa proseguire la ricerca con l'aggiunta di altri 5 neuroni
#ifdef PRINT_STDOUT
      printf("\n THREAD NUMBER %d , Good performances, the search goes on\n\n",
             ingresso->thread_number);
#endif

#ifdef PRINT_REPORT
      sprintf(line, "\n    Good performances, the search goes on\n\n");
      update_thread_buffer(ingresso, line);
#endif
    }

  } // end for
  fann_destroy_train(testing_data);
  fann_destroy_train(training_data);
  fann_destroy(ann);
  return NULL;
} // end compute_shuffle

#ifdef PRINT_REPORT
// Funzione utilizzata in "compute_shuffle", che si occupa di riempire il buffer
// del relativo thread con i messaggi di report forniti. In caso di overflow del
// buffer, la funzione si occupa del riallocamento di memoria necessario.
// Parametri di ingresso:	ingresso = puntatore alla struttura del thread
// che contiene il puntatore al buffer da riempire(si veda la struct shuffle_in)
//				stringline = array contenente la stringa da
// inserire nel buffer Parametro di uscita:		void.
void update_thread_buffer(shuffle_in *ingresso, char stringline[]) {
  ingresso->lunghezza_tot +=
      strlen(stringline); // aggiornamento della lunghezza_tot della stringa
                          // contenuta nel buffer

  if (ingresso->lunghezza_tot >=
      ingresso->dimensione_buffer) // se la lunghezza totale è più grande (o al
                                   // limite uguale) della dimensione del
                                   // buffer, si alloca la dimensione del buffer
                                   // di altri buffer_size caratteri....
  {
    ingresso->dimensione_buffer +=
        buffer_size; // si aggiorna la reale dimensione del buffer
    ingresso->buffer_session =
        realloc(ingresso->buffer_session,
                ingresso->dimensione_buffer); // si alloca memoria pari alla
                                              // dimensione del buffer
  }

  strcat(ingresso->buffer_session,
         stringline); // si inserisce la stringa (in genere rappresentante una
                      // riga) nel buffer
}
#endif

// Funzione utilizzata per copiare la struttura della miglior rete trovata dalla
// funzione "compute_shuffle". Parametri di ingresso:	net = struttura della
// miglior rete da copiare. 				thread_num = numero
// identificativo del thread a cui
// associare la rete.
// Parametro di uscita:		struct fann* = puntatore alla struttura che
// identifica la copia della rete.
struct fann *my_fann_copy(struct fann *net, unsigned int thread_num) {
  char tmpnetfilename[20];
  sprintf(tmpnetfilename, "tempnetwork%d", thread_num);
  fann_save(net, tmpnetfilename);
  struct fann *ann = fann_create_from_file(tmpnetfilename);
  remove(tmpnetfilename);
  return ann;
}

// Procedura utilizzata per stampare a video un help relativo all'utilizzo del
// programma. Le istruzioni vengono visualizzate quando i parametri passati in
// ingresso sono errati, ovvero quando il programma non viene utilizzato
// correttamente.
void show_instruction() {
  printf("\n You can use this program for the following tasks:\n\n");
  printf(" 1) Training a neural network with Cascade2 algorithm and compute "
         "output data using the network\n");
  printf(" In this case, you can use --all or -a option:\n");
  printf(" %s --all training_filename input_to_compute network_filename "
         "output_filename 'constr'\n",
         program_name);
  printf(" Where:\n");
  printf(" '%s' is the name of the program you are running\n", program_name);
  printf(" 'training_filename' is the filename containing training data\n");
  printf(" 'input_to_compute' is the filename containing only input data you "
         "want to compute output with the network\n");
  printf(" 'network_filename' is the filename to save neural network\n");
  printf(" 'output_filename' is the filename to save results\n");
  printf(
      " 'constr' is the constraint to search the network. It should be choosen "
      "between FAST HIGH MEDIUM LOW. Please see documentation for details\n\n");
  printf(" 2) Training a neural network with Cascade2 algorithm without "
         "compute output data\n");
  printf(" In this case, you can use --train or -t option:\n");
  printf(" %s --train training_filename network_filename 'constr'\n",
         program_name);
  printf(" Where:\n");
  printf(" '%s' is the name of the program you are running\n", program_name);
  printf(" 'training_filename' is the filename containing training data\n");
  printf(" 'network_filename' is the filename to save neural network\n");
  printf(
      " 'constr' is the constraint to search the network. It should be choosen "
      "between FAST HIGH MEDIUM LOW. Please see documentation for details\n\n");
  printf(" 3) Compute output data using a previously build neural network.\n");
  printf(" In this case, you can use --predict or -p option:\n");
  printf(" %s --predict input_to_compute output_filename network_filename1 "
         "network_filename2 ....\n",
         program_name);
  printf(" Where:\n");
  printf(" '%s' is the name of the program you are running\n", program_name);
  printf(" 'input_to_compute' is the filename containing only input data you "
         "want to compute output with the network\n");
  printf(" 'output_filename' is the filename to save results\n");
  printf(" 'network_filename' filename of the network to use to compute output "
         "data\n\n");
  printf(" You can predict more models at the same time, so you can pass many "
         "neural network files. All results will be put in the same output "
         "file.\n\n");
  printf(" Enjoy!\n\n");
}

// Funzione utilizzata per salvare su file i valori min e max delle uscite
// (letti dal file di training). Tali valori sono utili per denormalizzare i
// risultati quando il programma viene utilizzato per calcolare i risultati
// usando una rete precedentemente elaborata. Nota: Nella nostra
// implementazione, il nome del file contentente questi valori è il nome del
// file della rete seguito da _minmax. Esempio: se in nome della rete è model1,
// il file contenente il min e il max delle uscite di quel modello è
// model1_minmax Parametri di ingresso:	file = nome del file in cui memorizzare
// il minimo
// e il massimo. 				min = valore minimo.
// max = valore massimo.
void save_minmax(char *file, fann_type min, fann_type max) {
  FILE *fd = fopen(file, "w");
  fprintf(fd, "%f\n", min);
  fprintf(fd, "%f", max);
  fclose(fd);
}

// Funzione utilizzata per caricare da file il minimo e massimo precedentemente
// salvati. Tali valori saranno utilizzati per denormalizzare i risultati
// calcolati dalla rete prima di scriverli su file.
// Parametri di ingresso:	file = nome del file da cui leggere il minimo e
// il
// massimo. 				min = puntatore in cui memorizzare il
// valore minimo letto dal file.
//				max = puntatore in cui memorizzare il valore
// massimo letto dal file. Parametro di uscita:		void.
void load_minmax(char *file, fann_type *min, fann_type *max) {
  FILE *fd = fopen(file, "r");
  if (fd == NULL) {
    /* L'apertura del file è fallita. Stampo un messaggio di errore ed esco dal
     * programma */
    printf("Error opening file containing min max values: %s. This file is "
           "necessary for denormalization. Aborting program...\n\n",
           file);
    exit(1);
  }
  fscanf(fd, "%f", min);
  fscanf(fd, "%f", max);
  fclose(fd);
}
