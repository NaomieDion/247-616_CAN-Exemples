#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <signal.h>
#include <sys/types.h>

#define CAN_ID 0x475  // Remplacez par les 3 derniers chiffres de votre numéro DA

void handle_receive(int fdSocketCAN)
{
    struct can_frame frame;
    int nbytes;

    while (1) 
	{
		/// A filter matches, when <received_can_id> & mask == can_id & mask
		struct can_filter rfilter[1]; // filtres pour 2 ID

		rfilter[0].can_id   = 0x009; 
		rfilter[0].can_mask = 0x00F;
	    rfilter[1].can_id   = 0x274;
		rfilter[1].can_mask = 0xFFF;

		setsockopt(fdSocketCAN, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

        // Lecture d'un message CAN
        nbytes = read(fdSocketCAN, &frame, sizeof(struct can_frame));
        if (nbytes < 0) 
		{
            perror("Erreur de réception");
            exit(1);
        }
		printf("Message reçu - ID: %X, Data: ", frame.can_id);
        for (int i = 0; i < frame.can_dlc; i++) 
		{            
			printf("%02X ", frame.data[i]);
        }
        printf("\n");
    }
}

int main()
{
    int fdSocketCAN;
    struct sockaddr_can addr;
    struct ifreq ifr;
	//struct can_frame frame;
    pid_t pid;

    // Création du socket CAN
    if ((fdSocketCAN = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
        perror("Socket");
        return 1;
    }

    // Configuration de l'interface CAN (par défaut : can0)
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(fdSocketCAN, SIOCGIFINDEX, &ifr) < 0) 
	{
        perror("Erreur d'ioctl");
        return 1;
    }
	memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Liaison du socket à l'interface CAN
    if (bind(fdSocketCAN, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Erreur de liaison du socket");
        return 1;
    }

    // Création du processus fils
    pid = fork();
    if (pid == 0)
	 {  // Processus fils pour la réception des messages
        handle_receive(fdSocketCAN);
    } 
	else if (pid > 0) 
	{  // Processus père pour l'envoi des messages
        struct can_frame frame;
        frame.can_id = CAN_ID;  // Utilisation des trois derniers chiffres de DA comme ID
        frame.can_dlc = 7;		// nombre d'octets de données

        while (1) 
		{
            int choix;
            printf("Menu :\n1. Envoyer un message CAN\n2. Quitter\nChoix : ");
            scanf("%d", &choix);

        if (choix == 1) {
                sprintf((char *)frame.data, "616-TGE");  // Données
				//frame.data[0] = 0xFF;
                if (write(fdSocketCAN, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) 
				{
                    perror("Erreur d'envoi");
                } 
				else 
				{
                    printf("Message envoyé - ID: %X, Data: %s\n", frame.can_id, frame.data);
                }
            } else if (choix == 2) {
                kill(pid, SIGTERM);  // Terminer le processus fils
                break;
            }
        }
    }
	else 
	{
        perror("Erreur de fork");
        return 1;
    }

    close(fdSocketCAN);
    return 0;
}