#include "server_utils.h"
#include "general.h"

using namespace std;
bool create_pck(udp_pck *topic, tcp_pck *pck)
{
    DIE(topic->type > 3, "Packetul primit are continut incorect!\n");

    strncpy(pck->topic_name, topic->topic_name, 50);
    pck->topic_name[50] = 0; // asigura terminarea stringului

    long long integer;
    double real_num;

    switch (topic->type)
    {
    case 0:
        strcpy(pck->type, "INT");
        // INT (Numar intreg fara semn)
        // Octet de semn* urmat de un uint32_t formatat conform network byte order
        DIE(topic->data[0] > 1, "Packetul primit are continut incorect!\n")

        integer = ntohl(*(uint32_t *)(topic->data + 1)); // valoarea fara semn
        if (topic->data[0])
        {
            integer *= -1;
        }
        sprintf(pck->data, "%lld", integer);

        break;

    case 1:
        strcpy(pck->type, "SHORT_REAL");
        // SHORT_REAL (Numar real pozitiv cu 2 zecimale)
        // uint16_t reprezentand modulul numarului inmultit cu 100
        real_num = ntohs(*(uint16_t *)(topic->data));
        real_num /= 100; // pentru a determina cele doua cifre de dupa virgula
        sprintf(pck->data, "%.2f", real_num);

        break;

    case 2:
        strcpy(pck->type, "FLOAT");
        // FLOAT (Numar real)
        // Un octet de semn, urmat de un uint32_t (in networkbyte order)
        // reprezentand modulul numarului obtinut din alipirea partii ıntregi de partea
        // zecimala a numarului, urmat de un uint8_t ce reprezinta modulul puterii
        // negative a lui 10 cu care trebuie ˆınmult , it modulul pentru a
        // obtine numarul original (ın modul)
        DIE(topic->data[0] > 1, "Packetul primit are continut incorect!\n") // 0

        real_num = ntohl(*(uint32_t *)(topic->data + 1)); // 1 - 4
        real_num /= pow(10, topic->data[5]);              // 5

        if (topic->data[0])
        {
            real_num *= -1;
        }

        sprintf(pck->data, "%lf", real_num);

        break;

    default:
        // STRING (Sir de caractere)
        // Sir de maxim 1500 de caractere, terminat cu \0 sau
        // delimitat de finalul datagramei pentru lungimi mai mici
        strcpy(pck->type, "STRING");
        strcpy(pck->data, topic->data);
        break;
    }

    return true;
}

void close_fds(fd_set *fds, int max_fd)
{
    for (int i = 2; i <= max_fd; ++i)
    {
        if (FD_ISSET(i, fds))
        {
            close(i);
        }
    }
}

// impartim pe nivele
vector<string> split_levels(string s)
{
    vector<string> parts;
    stringstream ss(s);
    string item;
    while (getline(ss, item, '/'))
    {
        parts.push_back(item);
    }
    return parts;
}

// comparam recursiv "nivelele" din topic cu cele din pattern
bool match_levels(vector<string> patternParts, int i,
                  vector<string> topicParts, int j)
{
    int P = patternParts.size();
    int T = topicParts.size();

    // Match
    if (i == P && j == T)
        return true;
    // no match
    if (i == P)
        return false;
    // only match if it's exactly one "*" at end
    if (j == T)
        return (patternParts[i] == "*" && i + 1 == P);

    string pat = patternParts[i];
    string top = topicParts[j];

    if (pat == "*")
    {
        // '*' matches zero or more levels
        if (match_levels(patternParts, i + 1, topicParts, j))
            return true;
        return match_levels(patternParts, i, topicParts, j + 1);
    }
    else if (pat == "+")
    {
        // '+' matches exactly one level
        return match_levels(patternParts, i + 1, topicParts, j + 1);
    }
    else
    {
        if (pat == top)
        {
            return match_levels(patternParts, i + 1, topicParts, j + 1);
        }
        else
        {
            return false;
        }
    }
}

// Return true if any pattern in 'patterns' matches 'topic'
bool contains_topic(unordered_set<string> patterns, string topic)
{
    vector<string> topicParts = split_levels(topic);
    for (string pat : patterns)
    {
        vector<string> patternParts = split_levels(pat);
        if (match_levels(patternParts, 0, topicParts, 0))
        {
            return true;
        }
    }
    return false;
}
