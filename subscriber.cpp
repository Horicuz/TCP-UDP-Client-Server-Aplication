#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <netinet/tcp.h>
#include "subscriber_utils.h"

using namespace std;

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    DIE(argc != 4, "Usage: ./subscriber <ID> <IP_SERVER> <PORT_SERVER>.\n")
    DIE(strlen(argv[1]) > 10, "Subscriber ID should be at most 10 characters long.\n")

    char buffer[MAX_RECV];

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(serv_sock < 0, "Error creating server socket (listen).\n")

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    DIE(inet_aton(argv[2], &serv_addr.sin_addr) == 0,
        "Incorrect <IP_SERVER>!\n")
    DIE(connect(serv_sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0,
        "Couldn't establish connection with the server!.\n")

    // dupa ce am reusit sa ne conectam la server, trimitem id-ul clientului,
    // inainte de a efectua orice fel de comanda
    DIE(send(serv_sock, argv[1], strlen(argv[1]) + 1, 0) < 0, "Couldn't send ID to server!\n")

    int flag = 1; // dezactivam algoritmul Nagle
    setsockopt(serv_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    fd_set active_fds, read_fds;
    FD_ZERO(&active_fds);
    FD_SET(serv_sock, &active_fds);
    FD_SET(0, &active_fds);

    while (1)
    {
        read_fds = active_fds;

        DIE(select(serv_sock + 1, &read_fds, nullptr, nullptr, nullptr) < 0,
            "Couldn't select!\n")

        if (FD_ISSET(0, &read_fds))
        {
            // am primit un mesaj de la stdin
            memset(buffer, 0, MAX_RECV);
            fgets(buffer, MAX_BUFFER_SIZE - 1, stdin);

            if (!strcmp(buffer, "exit\n"))
            {
                break;
            }

            // altceva decat exit
            serv_pck sent_msg;
            if (create_req(&sent_msg, buffer))
            {
                DIE(send(serv_sock, (char *)&sent_msg, sizeof(sent_msg), 0) < 0,
                    "Unable to send message to server.\n")

                // se afiseaza mesajele corespunzatoare comenzii date
                if (sent_msg.comm == 'S')
                {
                    printf("Subscribed to topic %s\n", sent_msg.topic_name);
                }
                else
                {
                    printf("Unsubscribed from topic %s\n", sent_msg.topic_name);
                }
            }
        }

        if (FD_ISSET(serv_sock, &read_fds))
        {
            // s-a primit un mesaj de la server (adica de la un client UDP al acestuia)
            memset(buffer, 0, MAX_RECV);

            size_t to_read = sizeof(tcp_pck), total = 0;

            int ok1 = 0;
            int first = 1;
            while (total < to_read)
            {
                ssize_t num_bytes = recv(serv_sock, buffer + total, to_read - total, 0);
                DIE(num_bytes < 0, "Couldn't receive from server!\n")

                if (num_bytes == 0 && first == 1)
                {
                    ok1 = 1;
                    break;
                }
                else
                {
                    first = 0;
                    total += num_bytes;
                }
            }

            // serverul a inchis clientul
            if (ok1 == 1)
            {
                break;
            }

            tcp_pck *recv_msg = (tcp_pck *)buffer;
            printf("%s:%hu - %s - %s - %s\n", recv_msg->ip, recv_msg->port, recv_msg->topic_name, recv_msg->type, recv_msg->data);
        }
    }

    close(serv_sock);

    return 0;
}