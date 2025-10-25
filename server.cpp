#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "general.h"
#include "server_utils.h"

using namespace std;

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(argc != 2, "Usage: ./server <PORT>")
    uint16_t port;
    port = atoi(argv[1]);
    DIE(port < 1024, "Incorrect port number.\n")

    // Create TCP Socket
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sock < 0, "Error creating TCP socket (listen).\n")

    // Enable address reuse for TCP socket
    int enable = 1;
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        DIE(1, "setsockopt(SO_REUSEADDR) failed");

    // Create UDP socket
    int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udp_sock < 0, "Error creating UDP socet.\n")

    sockaddr_in udp_addr, tcp_addr;

    udp_addr.sin_family = tcp_addr.sin_family = AF_INET;
    udp_addr.sin_port = tcp_addr.sin_port = htons(port);
    udp_addr.sin_addr.s_addr = tcp_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind UDP socket
    DIE(bind(udp_sock, (sockaddr *)&udp_addr, sizeof(sockaddr_in)) < 0,
        "Unable to bind UDP socket.\n")
    // Bind TCP socket
    DIE(bind(tcp_sock, (sockaddr *)&tcp_addr, sizeof(sockaddr_in)) < 0,
        "Unable to bind TCP socket.\n")

    // Listen on TCP socket for potential TCP Clients (No limit on the number of clients)
    DIE(listen(tcp_sock, INT_MAX) < 0, "Unable to listen on the TCP socket.\n")

    // The same as poll() and pollfd
    fd_set active_fds, read_fds;
    FD_ZERO(&active_fds);
    // FD_ZERO(&read_fds);

    // Adaugam canalele de comunicatie de pana acum (UDP, TCP si tastatura)
    FD_SET(tcp_sock, &active_fds);
    FD_SET(udp_sock, &active_fds);
    FD_SET(0, &active_fds);
    int max_fd = tcp_sock > udp_sock ? tcp_sock : udp_sock;

    // Atunci cand primim de la tastatura exit, serverul se opreste
    bool exit_flag = false;

    // data structures we need:
    // client id -> online/offline
    unordered_map<string, bool> online_id;

    // fd pentru fiecare client id
    // // client_id <-> fd
    unordered_map<string, int> clients_map;
    unordered_map<int, string> fd_map;

    // ce topicurile pentru fiecare client
    // client id -> topics
    unordered_map<string, unordered_set<string>> subscribers;

    char buffer[MAX_BUFFER_SIZE];

    while (!exit_flag)
    {
        read_fds = active_fds;
        memset(buffer, 0, MAX_BUFFER_SIZE);

        // we select the fd where we have data to read
        int info = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        DIE(info < 0, "Error on select.\n")

        for (int i = 0; i <= max_fd; ++i)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (!i) // am primit ceva de la tastatura
                {
                    fgets(buffer, MAX_BUFFER_SIZE - 1, stdin);

                    if (!strcmp(buffer, "exit\n"))
                    {
                        exit_flag = true;
                        break;
                    }
                    else
                    {
                        printf("Unrecognised input. Try Again!\n");
                    }
                }
                else if (i == udp_sock) // primire pachet UDP
                {
                    sockaddr_in udp_client;
                    socklen_t udp_client_len = sizeof(udp_client);

                    int udp_data = recvfrom(udp_sock, buffer, MAX_BUFFER_SIZE - 1, 0,
                                            (sockaddr *)&udp_client, &udp_client_len);
                    DIE(udp_data < 0, "Error receiving UDP packet.\n")

                    udp_pck *topic = (udp_pck *)buffer;
                    DIE(topic == NULL, "Error creating UDP packet.\n")

                    // Parsam topicul primit pentru a crea pachetul TCP
                    tcp_pck pck;
                    pck.port = ntohs(udp_client.sin_port);
                    strcpy(pck.ip, inet_ntoa(udp_client.sin_addr));

                    if (create_pck(topic, &pck))
                    {
                        // se trimite mesajul tuturor clientilor abonati la acest topic
                        for (auto sub : subscribers)
                        {
                            if (online_id[sub.first] == true)
                            {
                                if (contains_topic(sub.second, pck.topic_name))
                                {
                                    // clientul este online si abonat la topic
                                    // se trimite mesajul UDP catre clientul TCP
                                    DIE(send(clients_map[sub.first], (char *)&pck, sizeof(tcp_pck), 0) < 0,
                                        "Unable to send message to TCP client!\n")
                                }
                                else
                                {
                                    // clientul este online dar nu este abonat la topic
                                    continue;
                                }
                            }
                            else
                            {
                                // clientul nu este online dar preferintele lui raman
                                // in caz ca se va reabona
                                continue;
                            }
                        }
                    }
                }
                else if (i == tcp_sock) // primire (doar) cereri de conectare
                {
                    // acceptam un nou client TCP
                    sockaddr_in new_sub;
                    socklen_t new_sub_len = sizeof(new_sub);
                    int new_sock = accept(i, (sockaddr *)&new_sub, &new_sub_len);
                    DIE(new_sock < 0, "Could not accept new socket!\n")

                    // dezactivam algoritmul Nagle
                    int flag = 1;
                    setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

                    if (new_sock > max_fd)
                    {
                        max_fd = new_sock;
                    }

                    // primim id-ul clientului
                    DIE(recv(new_sock, buffer, MAX_BUFFER_SIZE - 1, 0) < 0, "Couldn't receieve from TCP client!\n")

                    string recv_id = string(buffer);

                    if (online_id.find(recv_id) != online_id.end())
                    {
                        if (online_id[recv_id] == true) // clientul e deja conectat
                        {
                            printf("Client %s already connected.\n", buffer);
                            close(new_sock);
                            continue;
                        }
                        else
                        {
                            // clientul s-a reconectat
                            online_id[recv_id] = true;
                            clients_map[recv_id] = new_sock;
                            fd_map[new_sock] = recv_id;
                            FD_SET(new_sock, &active_fds);
                            printf("New client %s connected from %s:%hu.\n", buffer, inet_ntoa(new_sub.sin_addr), ntohs(new_sub.sin_port));
                        }
                    }
                    else
                    {
                        // clientul e nou
                        online_id[recv_id] = true;
                        clients_map[recv_id] = new_sock;
                        fd_map[new_sock] = recv_id;
                        FD_SET(new_sock, &active_fds);
                        printf("New client %s connected from %s:%hu.\n", buffer, inet_ntoa(new_sub.sin_addr), ntohs(new_sub.sin_port));
                    }
                }
                else // primire mesaje de la un subscriber
                {
                    int tcp_rcv_bytes = recv(i, buffer, MAX_BUFFER_SIZE - 1, 0);
                    DIE(tcp_rcv_bytes < 0, "Couldn't receive from client!\n")

                    if (!tcp_rcv_bytes) // cand subscriber a folosit EXIT
                    {

                        // practic clientul ramane in listele de subscriberi ale topicurilor
                        //  doar ca fiind inactiv nu primeste mesajele
                        //  cand se va reconecta va fi din automat subscris la topicurile dorite
                        printf("Client %s disconnected.\n", fd_map[i].c_str());
                        online_id[fd_map[i]] = false;
                        FD_CLR(i, &active_fds);

                        for (int j = max_fd; j > 2; --j)
                        {
                            if (FD_ISSET(j, &active_fds))
                            {
                                max_fd = j;
                                break;
                            }
                        }

                        close(i);
                    }
                    else // s-a primit o comanda de subscribe sau unsubscribe
                    {
                        serv_pck *command = (serv_pck *)buffer;
                        string user_sub = fd_map[i];

                        if (command->comm == 'U')
                        {

                            if (subscribers[user_sub].find(command->topic_name) != subscribers[user_sub].end())
                            {
                                subscribers[user_sub].erase(command->topic_name);
                            }
                            else
                            {
                                // printf("Client %s is not subscribed to %s.\n", fd_map[i].c_str(), command->topic_name);
                            }
                        }
                        else
                        {
                            subscribers[user_sub].insert(command->topic_name);
                        }
                    }
                }
            }
        }
    }

    close_fds(&active_fds, max_fd);

    return 0;
}
