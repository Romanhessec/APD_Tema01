Things to fix:
    1. Variabile globale pentru sincronizare (mutex, barrier, etc);
    2. Variabila globala "done" - pentru sincronizarea reducerilor;
    3. Rezolva mizeria de la mutexul reducerilor;
    4. Vezi cine deschide exact fisierele - trebuie deschise din mappers
    5. Cand calculezi numarul de numere diferite, sterge dublurile peste care deja treci