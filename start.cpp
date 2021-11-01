#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <exception>
#include <random>
#include <list>
#include <map>
#include <cmath>
#include <chrono>
#include <ctime>
#include <signal.h>

using namespace std;


// ********  ********
//  ~ ~ ~ ALSP ~ ~ ~
// ********  ********


// - GLOBALES -

//semillas para random
int seed = time(NULL);

//variables
string fileNom = string("./Inst/") + string("airland2.txt"); //archivo con instancias
int maxIteraciones = 10000; //cantidad de iteraciones a realizar
int tabuS = 150;    //tamaño de lista Tabú
int paso = 30;      //tamaño de movimiento al buscar soluciones (cantidad constante)

int maxInfactibles = 3;  //cantidad máxima de soluciones infactibles seguidas
int max_conflicto = 30; //cantidad máxima de soluciones con la misma cantidad de conflictos


//constantes
bool factibleGlobal = false;  //si la solución hallada es factible
bool huboFactible = false;    //si en algún momento se encontró una solución factible
int nivel_fact = -9999999;    //nivel de factibilidad de la solución actual (según cantidad de conflictos)

int fact_previo = 0;  //nivel de factibilidad previo
int n_inf = 0;        //contador de soluciones infactibles seguidas (se usa en main)
int n_conf = 0;       //contador de soluciones con la misma cantidad de conflictos seguidas (se usa en main)
int inst = 0, ifs = 0;  //cantidad de instanciaciones y chequeos

vector <int> best_S;    //mejor solución
int best_C = 9999999;   //costo de mejor solución
int bestF = -99999999;  //mejor nivel de factibilidad de solución

vector<int>* tabuL;  //lista Tabú

chrono::duration<double> T_best; //tiempo para encontrar mejor solución

auto tiempo_i = std::chrono::system_clock::now(); //tiempo de inicio
// - - - - - - -


class Avion{

    public:

        vector <int> S; //separacion con demás aviones
        int E_i;        //earliest arrive
        int T_i;        //ideal
        int L_i;        //latest arrive
        float g_i;      //cost t < T_i
        float h_i;      //cost t > T_i

        //constructor
        Avion(vector<int> in_S, int in_E, int in_T, int in_L, float in_g, float in_h)
            :S(in_S), E_i(in_E), T_i(in_T), L_i(in_L), g_i(in_g), h_i(in_h)
        {}

        //se requiere un constructor por defecto para hacer vectores de clases
        Avion(){}
};


//escribir datos en archivo de texto
void writeData(fstream &df){

    auto tiempo_f = chrono::system_clock::now();

    if (huboFactible){
        int sizeS = best_S.size();
        int i;
        
        df << "Mejor solución: {";
        for (i = 0; i < sizeS - 1; i++){
            df << best_S[i] << ", ";
        }
        df << best_S[i] << "}" << endl;

        df << "Costo: " << best_C << endl;
    }

    df << "Cantidad de instancias (o iteraciones): " << inst << endl;
    df << "Cantidad de chequeos: " << ifs << endl;

    if (huboFactible){
        df << "Tiempo de búsqueda de mejor solución: " << T_best.count() << " s" << endl;
    }

    chrono::duration<double> T = tiempo_f - tiempo_i;
    df << "Tiempo de ejecución: " << T.count() << " s" << endl;
}


//imprimir datos por pantalla
void printData(){

    //tiempo final
    auto tiempo_f = chrono::system_clock::now();

    cout << "\n\n- - - - - - - - - - - - - - - - - - - - - -\n";
    if (huboFactible){
        int sizeS = best_S.size();
        int i;
        
        cout << "Mejor solución: {";
        for (i = 0; i < sizeS - 1; i++){
            cout << best_S[i] << ", ";
        }
        cout << best_S[i] << "}" << endl;

        cout << "Costo: " << best_C << endl;
    }

    cout << "Cantidad de instancias (o iteraciones): " << inst << endl;
    cout << "Cantidad de chequeos: " << ifs << endl;

    if (huboFactible){
        cout << "Tiempo de búsqueda de mejor solución: " << T_best.count() << " s" << endl;
    }
    chrono::duration<double> T = tiempo_f - tiempo_i;
    cout << "Tiempo de ejecución: " << T.count() << " s" << endl;
    cout << "- - - - - - - - - - - - - - - - - - - - - -\n";
}


//manejar ctrl + c
void my_handler(int s){
    printData();
    delete []tabuL;
    exit(1);
}


//retorna arreglo de strings separando la variable str por sus espacios.
string* split(string str){
  
    string* arr = new string[int(str.length()/2) - 1];
  
    //recorrer string (comienza en 1, porque los archivos
    //tienen espacios en cada inicio de linea)
    int i, j = 0;
    for (i = 1; str[i] != '\0'; i++) {
  
        if (str[i] == ' '){
            j++;
        }

        else {
            arr[j] += str[i];
        }
    }

    return arr;
}


//criterio de término para finalizar programa
bool criterioTermino(int instancias){
    
    //depende de cantidad de soluciones con la misma 
    //cantidad de conflictos seguidas o cantidad de repeticiones
    if (n_conf > max_conflicto || instancias > maxIteraciones){
        return true;
    }

    return false;
}


int len(int* Arr){
    return sizeof(Arr)/sizeof(Arr)[0];
}

int len(int** Arr){
    return sizeof(Arr)/sizeof(Arr)[0];
}


//imprimir solución
void printSol(vector<int> &Sol, string tag = "Solución"){

    int sizeS = Sol.size();
    int i;

    cout << tag << ": ";
    for (i = 0; i < sizeS; i++){
        cout << Sol[i] << " ";
    }
    cout << endl;
}


//pertenencia de solución en lista Tabú
bool checkTabu(vector<int> *Tabu, vector<int> &Sol){

    int solS = Sol.size();
    int i, j;
    for (i = 0; i < tabuS; i++){
        if (Sol == Tabu[i]){
            return true;
        }
    }    
    return false;
}


//evaluación de funcion objetivo a minimizar
//recibe aviones y una solución
float CostoFO(vector<Avion> &Aviones, vector<int> &Sol){

    float costo = 0;
    int T_llegada, T_ideal;

    int i;
    for (i = 0; i < Aviones.size(); i++){
        
        T_llegada = Sol[i];
        T_ideal = Aviones[i].T_i;
        
        //early costs
        if (T_llegada <= T_ideal){
            costo += Aviones[i].g_i * float(T_ideal - T_llegada);
        }

        //late costs
        else {
            costo += Aviones[i].h_i * float(T_llegada - T_ideal);
        }
    }

    return costo;
}


//verifica que se cumplan las distancias entre todos los aviones
bool distanciasAviones(vector<Avion> &Aviones, vector<int> &Sol){

    int sizeS = Sol.size();

    int i, j;
    for (i = 0; i < sizeS; i++){

        for (j = 0; j < sizeS; j++){

            //S es matriz simétrica
            if (j > i){

                //si no se respeta una de las distancias
		        if (abs(Sol[j] - Sol[i]) < Aviones[i].S[j]){
                    ifs++;
                	return false;
		        }
            }
        }
    }
    
    return true;
}


//verifica si los tiempos están dentro de las ventanas de tiempo
bool ventanasAviones(vector<Avion> &Aviones, vector<int> &Sol){

    int T_llegada, T_ideal;

    int i;
    for (i = 0; i < Aviones.size(); i++){
        
        T_llegada = Sol[i];
        
        if (!(T_llegada <= Aviones[i].L_i && T_llegada >= Aviones[i].E_i)){
            ifs++;
            return false;
        }
    }
    return true;
}


//retorna cantidad (negativa) de pares de aviones 
//que no respetan su distancia de tiempo de aterrizaje
int nivelFact(vector<Avion> &Aviones, vector<int> &Sol){

    int sizeS = Sol.size();
    int nivel = 0;

    int i, j;
    for (i = 0; i < sizeS; i++){

        for (j = 0; j < sizeS; j++){

            if (j > i){
                
                //nivel de factibilidad disminuye por cada 
                //restricción de distancia entre aviones violada (conflictos)
                if (abs(Sol[j] - Sol[i]) < Aviones[i].S[j]){
                    ifs++;
                	nivel -= 1;
		        }
            }
        }
    }
    return nivel;
}


//agregar una solución a lista Tabú
void addTabu(vector<int>* Tabu, vector<int> X){

    int i = 0, j;
    while (i < tabuS && Tabu[i].size() > 0) {
        i++;
    }
    
    //si la lista está completa
    if (i == tabuS){

        i = tabuS - 1; //posición a reemplazar

        for (j = 0; j < (tabuS - 1); j++){
            Tabu[j] = Tabu[j + 1];
        }
    }
    
    Tabu[i] = X;
}


//buscar mejor vecino verificando lista tabú
vector<int> escogerVecino(vector<Avion> &Aviones, vector<int>* Tabu, vector <vector<int>> &Vecindario, bool rankingFactibilidad = false){

    //activada al encontrar una solución factible,
    //para que solo admita de este tipo posteriormente
    bool factible = false;
    
    //activada cuando se encuentra una solución factible después de estar en infactibles
    bool nuevoFact = false;

    int sizeV = Vecindario.size();

    vector<int> bestV;       //mejor vecino
    float bestC = 99999999;  //costo de FO de mejor vecino
    float costoV;            //costo de FO del vecino analizado

    int factV;               //nivel de factibilidad del vecino analizado

    int i;
    for (i = 0; i < sizeV; i++){

        //escoge un vecino que no sea tabú, que sea factible y con menor costo
        //edit: haré que respetar las ventanas sea condición para seleccionar.
        if (!checkTabu(Tabu, Vecindario[i]) && ventanasAviones(Aviones, Vecindario[i])){
            
            //si se encontró una solución factible en el vecindario,
            //solo se buscarán vecinos factibles a partir de ese punto
            if (factible){

                //si es factible
                if (distanciasAviones(Aviones, Vecindario[i])){

                    costoV = CostoFO(Aviones, Vecindario[i]);
                    
                    if (costoV < bestC){
                        bestV = Vecindario[i];
                        bestC = costoV;
                    }
                }
            }

            //si no se han hallado soluciones factibles en el vecindario
            else {
                
                //si se buscan soluciones factibles en vez de las mejores
                if (rankingFactibilidad){

                    //nivel de factibilidad de la solución
                    factV = nivelFact(Aviones, Vecindario[i]);
                    
                    //se escoge el que tenga mayor nivel de factibilidad
                    if (factV >= bestF){

                        bestV = Vecindario[i];
                        bestF = factV;
                    }
                    
                    //si tiene igual factibilidad que la anterior o hay un solo 
                    //conflicto de factibilidad (o ninguno), se escoge la mejor solución
                    else if (factV == bestF && factV == 0){
                        
                        costoV = CostoFO(Aviones, Vecindario[i]);
                        
                        if (costoV > bestC){
                            bestV = Vecindario[i];
                            bestC = costoV;
                        }   
                    }
                    //si se halla una solución factible.
                    //como se ve aquí, su objetivo es evitar evaluar 
                    //dos veces la factibilidad de una solución
                    if (factV == 0) nuevoFact = true;
                }

                //si, en cambio, se busca la mejor solución
                else {

                    costoV = CostoFO(Aviones, Vecindario[i]);

                    if (costoV < bestC){
                        bestV = Vecindario[i];
                        bestC = costoV;
                    }
                    
                    //si se halla una solución factible
                    if (distanciasAviones(Aviones, Vecindario[i])){
                        nuevoFact = true;
                    }
                }

                //cuando se halla una solución factible
                if (nuevoFact){
                    cout << endl << "Solución factible hallada!" << endl << endl;
                    
                    factible = true;
                    factibleGlobal = true;
                    huboFactible = true;

                    bestF = -99999999;
                }
            }
        }
    }
    
    //en caso de haber estado buscando una solución factible
    //y que se haya escogido a un vecino
    if (rankingFactibilidad && bestV.size() > 0){
        cout << "nivel de factibilidad: " << bestF << endl;
    }

    return bestV;
}


//recibe solución actual y cantidad de aviones (dimensión)
//genera un vecindario de soluciones factibles (tiempos dentro de ventanas) en cruz
vector<vector<int>> generarVecindario(vector<int> &S_Act, int p){
    
    int i, j;

    //número de vecinos
    int n_comb = 2 * p;

    vector <vector<int>> Vecindario (n_comb, vector<int> (p));

    //se comienza con el vecino "más lejano" en toda coordenada
    for (i = 0; i < n_comb; i += 2){
        
        Vecindario[i] = S_Act;
        Vecindario[i + 1] = S_Act;

        Vecindario[i][i/2] = S_Act[i/2] + paso;
        Vecindario[i + 1][i/2] = S_Act[i/2] - paso;
    }

    return Vecindario;
}


//solución inicial aleatoria dentro de los límites de tiempo de cada avión
vector<int> generarSolucionInicial(vector<Avion> &Aviones, int p){
    
    vector <int> init_sol (p);

    //límites de tiempo de aterrizaje
    int lim_inf, lim_sup;
    
    int i;
    for (i = 0; i < p; i++) {
        
        lim_inf = Aviones[i].E_i;
        lim_sup = Aviones[i].L_i;

        //se considera el caso en que los límites sean iguales
        if (lim_sup - lim_inf > 0){
            init_sol[i] = rand() % (lim_sup - lim_inf) + lim_inf;
        }
        else {
            init_sol[i] = lim_inf;
        }
    }

    return init_sol;
}


int main(){

    //semilla para random
    srand(seed);

    //handler para manejar señal de término (ctrl + c)
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);


    //abrir archivo con instancias
    ifstream instancias (fileNom);
    if (!instancias.is_open()){
        throw "Error abriendo archivo de instancias.";
    }


//Leer archivo

    //cantidad de aviones
    int p;

    string line;
    //pasar string a int
    getline (instancias, line);
    p = stoi(line);

    //inicializar instancias de los aviones
    vector<Avion> Aviones(p);


    int in_E, in_T, in_L;             //variables de input
    float in_g, in_h;                 //variables de input
    vector <int> in_S (p);            //vector de separaciones de aviones
    string* varArr = new string[5];   //arreglo con variables de cada avion
    int i, j;
//Recorrer info. de cada avión
    for (i = 0; i < p; i++){

        //leer variables
        getline (instancias, line);
        
        varArr = split(line);
        in_E = stoi(varArr[0]);
        in_T = stoi(varArr[1]);
        in_L = stoi(varArr[2]);
        in_g = stof(varArr[3]);
        in_h = stof(varArr[4]);

        
        //leer distancias a otros aviones
        getline (instancias, line);
        string* S_str = split(line);

        for(j = 0; j < p; j++){
            in_S[j] = stoi(S_str[j]);
        }

        delete [] S_str;


        //construir avión
        Aviones[i] = Avion(in_S, in_E, in_T, in_L, in_g, in_h);
    }

    instancias.close();
    delete [] varArr;


//Tabu
    
    //lista Tabú con vectores de soluciones
    tabuL = new vector<int>[tabuS];


//Generar soluciones

    vector <int> S_Act (p);      //solución actual
    int costo_Act = 999999999;   //costo actual

    //solución inicial
    S_Act = generarSolucionInicial(Aviones, p);
    inst++;
    
    //imprimir solución
    printSol(S_Act, "Solución inicial");

    //costo según Función Objetivo
    costo_Act = CostoFO(Aviones, S_Act);
    cout << "Costo solución: " << costo_Act << endl << endl;

    //guardar en Tabu
    addTabu(tabuL, S_Act);

    //mejor solución si es factible
    if (distanciasAviones(Aviones, S_Act)){
        
        best_S = S_Act;
        best_C = costo_Act;
        
        //archivo para escribir datos
        fstream datafile ("data.txt", ios::ate | ios::trunc | ios::in | ios::out);
        writeData(datafile);
        datafile.close();
    }
   
   
//Buscar otras soluciones

    vector <vector<int>> Vecindario;

    while (!criterioTermino(inst)){
    
        //guardar último nivel de factibilidad
        fact_previo = bestF;


        //generar vecindario
        Vecindario = generarVecindario(S_Act, p);


        //escoger vecino
        if (n_inf <= maxInfactibles){
            S_Act = escogerVecino(Aviones, tabuL, Vecindario);
        }

        //en caso de sobrepasar la tolerancia de soluciones infactibles seguidas
        else {
            S_Act = escogerVecino(Aviones, tabuL, Vecindario, true);
        }
        inst++;

        printSol(S_Act, "Solución vecina escogida");

        //en caso de no escoger ninguno
        if (S_Act.size() <= 0){
            inst--;
            cout << "No se escogió a ningún vecino" << endl;
            break;
        }

        //se imprime la factibilidad de la solución
        if (factibleGlobal){
            cout << "[factible]" << endl;
            n_inf = 0;
        }
        else {
            cout << "[infactible]" << endl;
            n_inf++;
        }
        
        //si el nivel de factibilidad previo es igual al actual o no
        if (bestF == fact_previo) n_conf++;
        else n_conf = 0;


        //costo según Función Objetivo
        costo_Act = CostoFO(Aviones, S_Act);
        cout << "costo solución: " << costo_Act << endl;

        //guardar en Tabu
        addTabu(tabuL, S_Act);

        //factibleGlobal asegura que la mejor solución sea factible
        if (costo_Act < best_C && factibleGlobal){
            
            best_C = costo_Act;
            best_S = S_Act;

            T_best = chrono::system_clock::now() - tiempo_i;
            
            printSol(S_Act, "Nueva mejor solución");
        }

        cout << endl;

        //guardar la mejor solución hasta el momento, si es que ha habido una
        if (huboFactible){

            fstream datafile ("data.txt", ios::ate | ios::trunc | ios::in | ios::out);
            writeData(datafile);
            datafile.close();
        }

        if (factibleGlobal){
            factibleGlobal = false;
        }
    }
    
    printData();
    delete [] tabuL;
    
    return 0;
}
