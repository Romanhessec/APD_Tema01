/* Roman Gabriel-Marian, 333CB *\

    Pentru inceput, functiile get_args si parse_in parseaza inputul (numarul de mapperi,
de reduceri, fisierul de input, numarul de fisiere ulterioare).
    Pentru functionalitatea mapperilor si a reducerilor, m-am folosit de doua structuri,
args_reducer si, respectiv, args_mapper, structuri care vor fi trimise ca argument pentru
functiile threadurilor (mapper_function pentru mapper si reducer_function pentru reducers).
Voi descrie in continuare campurile din aceste structuri (inafara de cele self-explanatory):
    1. args_mapper:
        - bool* finished = boolean folosit pentru a semnala faptul ca fiecare thread mapper
    s-a terminat, lucru ce va porni reducerii (folosesc un boolean finished in main si il
    trimit prin adresa in fiecare structura);
        - char** input_files = vector cu toate fisierele de input, urmand ca fiecare thread 
    de mapper sa-si calculeze static fisierele pe care trebuie sa le deschida/parseze.
        - struct node** mapper_results = array de liste de rezultate ale mapperilor; pentru 
    fiecare reducer exista o astfel de lista; astfel, daca avem 4 reduceri, mapper_results va 
    reprezenta un array de dimensiune 4 de liste;

    2. args_reducer:
        - bool* finished = acelasi boolean de mai sus;
        - struct node*** mapper_results = matrice de liste de rezultate ale mapperilor - atentie!
    a nu se confunda cu mapper_results de la args_mapper; basically, in main aloc un 
    struct node*** mapper_results, reprezentand chiar matricea ce va fi transmisa mai departe 
    fiecarui reducer - pentru fiecare mapper nu se transmite toata matricea, ci doar
    mapper_results[i], unde i reprezinta indexul/id-ul mapperului - tocmai de aceea in args_mapper
    avem un vector si in args_reducer o matrice; args_reducer are nevoie de toata matricea deoarece
    threadurile de reduceri au nevoie de rezultatele tuturor mapperilor pentru a putea calcula 
    outputul corect; pentru o explicatie mai buna, ofer urmatorul exemplu:
        mapper_results[i][j] este o lista cu rezultatele mapperului cu id i pentru numerele perfecte 
    cu exponent j (atentie! daca j == 0 este vorba despre numerele cu exponent 2, daca j == 1 este 
    vorba despre numerele cu exponent 3 etc, motivul fiind reprezentat de logica enuntului; am ales sa
    implementez asa pentru a nu lasa loc liber degeaba pentru j == 0 si j == 1).
        - struct node** aggregate_list = vector de liste cu rezultatelele pentru fiecare id de reducer;
    astfel, aggregate_list[i] reprezinta toate rezultatele ale tuturor mapperilor (deci rezultatele
    dupa parsarea/procesarea tuturor fisierelor de input) pentru exponentul i (cu mentiunea discutiei 
    despre exponent de mai sus); lista este folosita ulterior pentru a calcula numarul de numere putere 
    perfecta pentru exponentul corespunzator, informatie ce va fi scrisa in fisierele de output;

    Odata explicate aceste structuri, logica din spatele temei este mult mai usor de inteles. Toate 
threadurile vor porni in acelasi timp in main insa reducerii vor fi fortati sa astepte pana cand toti 
mapperi se termina (reamintesc variabila bool *finished). De asemenea, am folosit aici un mutex pentru a 
bloca initial doar primul reducer care ajunge mai devreme la verificare (while (!*(args_reducer->finished));).
In functia specifica mapperilor, fiecare mapper va parsa fisierul atribuit si va calcula numerele de putere
perfecta pentru fiecare exponent. Pentru algoritmul de perfect (is_perfect), vedeti mentiunea [1].
    Dupa ce toti mapperi isi termina functionalitatea (lucru de care m-am asigurat prin folosirea unei 
bariere), reducerii incep sa calculeze acea lista de agregare despre care am vorbit mai sus si sa calculeze,
respectiv scrie outpurile necesare (functia write_in_file).
    Intr-un final, asa cum se specifica si in enunt, threadurile isi dau join simultan, joinunidu-se in
aceeasi iteratie.

[1] Algoritmul a fost gasit pe linkul https://stackoverflow.com/questions/20730053/algorithm-to-find-nth-root-of-a-number.
Ultimul raspuns ofera si un github sursa (https://github.com/michel-leonard/C-MathSnip/blob/main/simple_functions/nth_root.c)
cu doua variante (tehnic trei, doar doua fiind eficiente) de algoritm care calculeaza nth root pentru un numar. 
Bineinteles, algoritmul va oferi doar o aproximare integer pentru anumite inputuri care nu au nth root natural. 
Tocmai de aceea, in functia mea is_perfect, dupa rularea algoritmului inmultesc outputul cu el insusi de n ori
(exponentul) pentru a verifica daca numarul raspuns al algoritmului este o aproximare sau un raspuns corect.