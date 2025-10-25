#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstdint>

/*
 * Macro pentru verificarea erorilor în C++
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
// topic (50) + type (1) + paylad (1500)
#define MAX_BUFFER_SIZE 1552
#define MAX_RECV (sizeof(tcp_pck) + 1)

#define DIE(assertion, call_description)             \
    if (assertion)                                   \
    {                                                \
        fprintf(stderr, "ERROR: " call_description); \
        exit(EXIT_FAILURE);                          \
    }

// just for testing purposes
#define CHECK(assertion, call_description) \
    if (assertion)                         \
    {                                      \
        fprintf(stderr, call_description); \
        return false;                      \
    }

// SERVER -> TCP
struct __attribute__((packed)) tcp_pck
{
    char ip[16];
    uint16_t port;
    char topic_name[51];
    char type[11];
    char data[1501];
};

// TCP -> SERVER
struct __attribute__((packed)) serv_pck
{
    char topic_name[51];
    char comm; // subscribe/unsubscribe topics
    // char id[11];  // id-ul clientului
};
