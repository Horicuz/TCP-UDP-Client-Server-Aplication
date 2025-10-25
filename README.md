Tema 2 "Protocoale de Comunicatie"  
Aplicatie client-server TCP si UDP pentru gestionarea mesajelor
Autor: Potop Horia-IOan

==============================  
1. INTRODUCERE  
Tema propune implementarea unui broker de mesaje de tip publish-subscribe, cu doua tipuri de componente:  
- serverul (broker), care primeste mesaje UDP de la publishers si le redirectioneaza catre abonatii TCP,  
- subscribers TCP, care trimit comenzi de subscribe/unsubscribe si primesc mesaje de la server.  

==============================  
2. COMPILARE SI RULARE  
Proiectul contine un Makefile simplu, cu urmatoarele comenzi:  
• `make server` - compileaza serverul (./server)  
• `make subscriber` - compileaza clientul TCP (./subscriber)  
• `make clean` - sterge fisierele obiect si executabilele  

Pentru lansare:  
• Server: `./server <PORT>`  
• Subscriber: `./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>`  

==============================  
3. FUNCTIONALITATE


• La pornire, serverul deschide simultan un socket UDP (receptionare mesaje publishers) si unul TCP (acceptare abonari).  
• Foloseste `select()` pentru a astepta pe cele doua socket-uri si pe stdin. 
• Singura comanda de la tastatura pentru server este comanda "exit" ce determina inchiderea serverului si a clientilor TCP conectati.

a) Componenta de redistribuire
• Dupa ce am creat serverul si am conectat clientii, iar unii din ei au ales topicurile de interes prin comenzi subscribe/unsubscribe
ramane sa fie realizata primirea pachetelor UDP si redirectionarea acestora doar la clientii ce sunt subscribed la acel topic.
• Acest lucru este realizat de functia "contains_topic" care verifica pt fiecare client daca mesajul de pe topicul primit se   
regaseste printre topicurile de interes
Wildcard "*" inlocuieste zero sau mai multe niveluri de topic, "+" inlocuieste exact un nivel. Fiecare mesaj ajunge o singura data la fiecare client, chiar daca mai multe abonari ii corespund aceluiasi topic.
• Cu ajutorul "create_pck", mesajele sunt decodificate conform formatului de topic, tip si payload (INT, SHORT_REAL, FLOAT, STRING), apoi impachetate intr-o structura TCP
si retransmise fiecarui abonat online al topicului respectiv.  
• Mesajele sunt trimise pe fiecare socket in parte si informatia primita de client este afisata pe ecran.

b) Componenta de creare conexiune
• Noile cereri de conectare ale clientilor TCP sunt verificate de server:
    -Un client nou este adaugat impreuna cu file descriptorul aferent intr-o structura de date de tip map,
    si se asteapta comenzi de subscribe/unsubscribe sau exit de la acesta.
    Pana atunci acesta nu are niciun topic favorit si nu primeste niciun mesaj
    -Un client vechi redevine activ, componenta "online" fiind acum false->true. In acest caz
    toate topicurile de interes adaugate in conexiuni anterioare, redevin active pentru acesta (erau pastrate).
    -Un client deja conectat incearca sa se conecteze din nou, serverul indicandu-i ca nu se poate realiza acest lucru.

c) Componenta de abonare,dezabonare si deconectare
• Pentru fiecare abonare TCP ("subscribe <topic>" / "unsubscribe <topic>"), serverul actualizeaza structurile interne de date si trimite subscriber-ului un mesaj de confirmare pe stdout. 
• Aceste comenzi sunt verificate ca pot fi realizate (unsubscribe de la un topic neexistent), iar clientul devine pregatit sa primeasca mesajele dorite.
• La deconectare, serverul inchide socket-ul TCP si seteaza clientul ca si "offline" in structura de date, in ideea unei posibile reconectari sub acelasi id.

De asemenea, din perspectiva fisierului "subscriber.cpp" functionalitatea este de asemenea impartita in 3 componente:

a) Componenta de conectare
•Activam clientul si asteptam sa fim acceptati de server. Apoi trimitem un mesaj de conectare in care includem si ID-ul clientului pentru a ne putea identifica.

b) Componenta de trimitere comenzi
•Aici avem 3 comenzi:
    -subscribe <topic> - ne aboneaza la un topic
    -unsubscribe <topic> - ne dezaboneaza de la un topic
    -exit - inchide conexiunea TCP cu serverul
• Aici avem functia "create_req" ce se ocupa de parsarea mesajului de pe STDIN si crearea unui pachet de trimis serverului.
• La inchidere, clientul TCP trimite un mesaj de deconectare serverului si inchide socket-ul.

c) Componenta de primire si afisare
• Clientul TCP primeste de la server mesajele legate de topicurile la care s-a abonat si le afiseaza ordonat pe stdout.

==============================  
4. ARHITECTURA SI STRUCTURILE DE DATE  
Serverul retine:  
• mapparea ID_CLIENT → file descriptor TCP → stare online/offline,  
• mapparea client → set de topics,  
• functii de potrivire pattern/topic cu suport pentru wildcard.  

Protocolul (tcp_pck) aplicatie-TCP este implementat printr-o structura compacta, marcata `packed`, care contine adresa IP si portul de origine UDP, numele topicului, tipul de date text si valoarea in format ASCII.  
Iar serv_pck este structura de date ce contine comanda dorita de client si topicul dorit.

==============================  
5. VALIDARI SI TRATAREA ERORILOR
• Toate apelurile de sistem (socket, bind, listen, accept, select, recvfrom, recv, send, setsockopt) sunt verificate, si pe eroare programul se inchide cu mesaj sugestiv pe stderr (macro DIE).  
• Buffer-ul de stdout este dezactivat (`_IONBF`) pentru ca mesajele de log si feedaback sa ajunga imediat, fara buffering.  
• Se dezactiveaza algoritmul Nagle (`TCP_NODELAY`) pe toate conexiunile TCP pentru a minimiza latenta trimiterii mesajelor scurte.  
• Comenzile de subscribe/unsubscribe sunt validate: exact doua token-uri, topic ≤ 50 caractere, fara tokene in plus, cu trimiteri de eroare la input invalid.  
• Pachetele UDP sunt decodificate cu grija la byte-order (ntohs/ntohl) si structurile rezultate au intotdeauna terminatoare de sir pentru topicuri mai scurte de 50 caractere.  

==============================  
6. ALTE INFORMATII
• Clientii isi pastreaza eficient topicurile in urma unei deconectari urmate de o reconectare.
• Implementarea acopera integral cerintele de: multiplexare TCP+UDP, wildcard-uri, protocol eficient de nivel aplicatie, management robust al erorilor si deconectarilor, fara mesaje suplimentare. Codul este modular si documentat, iar Makefile-ul simplifica compilarea.  
