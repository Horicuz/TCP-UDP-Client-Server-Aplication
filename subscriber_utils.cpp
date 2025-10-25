#include "subscriber_utils.h"

bool create_req(serv_pck *message1, char *buff)
{
    buff[strcspn(buff, "\n")] = '\0';
    char *token = strtok(buff, " ");
    CHECK(token == nullptr,
          "Commands are `subscribe <topic>`, `unsubscribe <topic>`, or `exit`!\n")

    // subscribe\unsubscribe
    if (strcmp(token, "subscribe") == 0)
    {
        message1->comm = 'S';
    }
    else if (strcmp(token, "unsubscribe") == 0)
    {
        message1->comm = 'U';
    }
    else
    {
        CHECK(true, "Commands are `subscribe <topic>`, `unsubscribe <topic>`, or `exit`!\n")
    }

    token = strtok(nullptr, " ");
    CHECK(token == nullptr,
          "Commands are `subscribe <topic>`, `unsubscribe <topic>`, or `exit`.\n")

    // Verificam ca nu mai avem extra-junk dupa topic name
    char *extra = strtok(nullptr, " ");
    CHECK(extra != nullptr,
          "Commands must be exactly two words: `subscribe <topic>`\n")

    // Enforce length ≤ 50
    size_t len = strlen(token);
    CHECK(len > 50, "Max 50 char for topic name!\n")

    memcpy(message1->topic_name, token, len);
    message1->topic_name[len] = '\0';

    return true;
}
