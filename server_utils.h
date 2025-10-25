#pragma once

#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <cstring>
#include <string>
#include <netinet/in.h>
#include <cmath>
#include <sstream>
#include <zconf.h>
#include <arpa/inet.h>
#include "general.h"

using namespace std;

// mesaj primit de SERVER de la UDP
// UDP -> SERVER
struct __attribute__((packed)) udp_pck
{
    char topic_name[50];
    uint8_t type;
    char data[1501];
};

bool create_pck(udp_pck *, tcp_pck *);

void close_fds(fd_set *, int);

vector<string> split_levels(string);
bool match_levels(const vector<string>, int, const vector<string>, int);
bool contains_topic(unordered_set<string>, string);
