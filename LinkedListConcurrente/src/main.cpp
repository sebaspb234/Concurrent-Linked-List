#include <vector>
#include <random>
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
using namespace std;


int get_random(int low, int high) { // Funcion que genera numeros aleatorios para hacer las pruebas
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(low, high);
    return distribution(gen);
}

template <class T>
struct CNode
{
    T value; // valor del nodo
    CNode<T>* next;
    atomic<bool> marked = false; // variable atomica que indica si un nodo esta siendo eliminado
    recursive_mutex mut; // recursive mutex para hacer lock y evitar problemas de interlock
    void lock()
    {
        mut.lock();
    }
    void unlock()
    {
        mut.unlock();
    }
    CNode(T x)
    {
        value = x;
        next = nullptr;
    }
};

template <class T>
struct LinkedList
{
    CNode<T>* head, * tail;
    LinkedList();
    ~LinkedList();
    bool insert(T x); // Funcion para insertar un nodo
    bool search(T x, CNode<T>*& pred, CNode<T>*& succ); // Funcion para buscar un nodo y apuntar al predecesor y sucesor del nodo
    bool remove(T x); // Funcion para eliminar un nodo
    void print(); // Funcion para imprimir la lista
};

template <class T>
LinkedList<T>::LinkedList()
{
    head = new CNode<T>(-214748368); // Inicializo head con el menor valor que se le puede asignar a un entero para evitar que se inserte o elimine el valor del head en la lista
    tail = new CNode<T>(214748368); // Inicializo tail con el mayor valor que se le puede asignar a un entero para evitar que se inserte o elimine el valode del tail en la lista
    head->next = tail; // Uno el nodo head al nodo tail
}

template <class T>
LinkedList<T>::~LinkedList()
{
    CNode<T>* t = nullptr;
    for (CNode<T>* p = head->next; p && p != tail; p = p->next)
    {
        t = p;
        remove(t->value);
    }
}

template <class T>
bool LinkedList<T>::search(T x, CNode<T>*& pred, CNode<T>*& succ)
{
    pred = head; // pred y succ apuntan a la cabeza y al nodo siguiente respectivamente
    succ = pred->next;
    while (succ->value < x) // Voy avanzando en la lista mientras el valor actual sea menor al valor buscado
    {
        pred = succ;
        succ = pred->next;
    }
    if (succ->value == x) return true; // Al salir del while retorna true si el valor de succ es el valor buscado
    return false; // Si no lo encontro, retorna false
}

template <class T>
bool LinkedList<T>::insert(T x)
{
    CNode<T>* pred = nullptr;
    CNode<T>* succ = nullptr;
    bool encontrado = false; // se guarda el resultado que devolverá search
    bool res = false; // indica si se pudo realizar la inserción o no

    while (1)
    {
        encontrado = search(x, pred, succ); // se busca el nodo con el valor x
        pred->lock(); // se bloquea al predecesor y sucesor
        succ->lock();
        if (!(pred->marked) && !(succ->marked) && pred->next == succ) // se comprueba que el predecesor y sucesor no estén siendo eliminados y que se mantengan conectados
        {
            if (encontrado) // si el valor fue encontrado, res es false, no se realizará la inserción
            {
                res = false;
            }
            else // si no lo encuentra, se inserta el nuevo nodo y res se vuelve true
            {
                CNode<T>* nuevoNodo = new CNode<T>(x);
                nuevoNodo->next = succ;
                pred->next = nuevoNodo;
                res = true;
            } // luego de la inserción se desbloquean los nodos pred y succ y se retorna res.
            pred->unlock();
            succ->unlock();
            return res;
        }// si la inserción no fue válida, se desbloquean los nodos que fueron bloqueados al inicio
        pred->unlock();
        succ->unlock();
    }
}

template <class T>
bool LinkedList<T>::remove(T x)
{
    CNode<T>* pred = nullptr;
    CNode<T>* succ = nullptr;
    bool encontrado = false; // se guarda el resultado que devolverá search
    bool res = false; // indica si se pudo realizar la inserción o no

    while (1)
    {
        encontrado = search(x, pred, succ); // se busca el nodo con el valor x
        pred->lock(); // se bloquea al predecesor y sucesor
        succ->lock();
        if (!(pred->marked) && !(succ->marked) && pred->next == succ) // se comprueba que el predecesor y sucesor no estén siendo eliminados y que se mantengan conectados
        {
            if (!encontrado) // si el valor no fue encontrado, res es false, no se realizará el borrado
            {
                res = false;
            }
            else // si el valor fue encontrado, se elimina de la lista y res se vuelve true
            {
                succ->marked = true;
                pred->next = succ->next;
                res = true;
            } // luego del borrado se desbloquean los nodos pred y succ y se retorna res.
            pred->unlock();
            succ->unlock();
            return res;
        }// si el borrado no fue válido, se desbloquean los nodos que fueron bloqueados al inicio
        pred->unlock();
        succ->unlock();
    }
}

template <class T>
void LinkedList<T>::print()
{
    cout << "HEAD -> ";
    for (CNode<T>* p = head->next; p && p != tail; p = p->next)
    {
        cout << p->value << " -> ";
    }cout << " NULL" << endl;
}


/// FUNCTORES PARA EL TEST DE OPERACIONES EN PARALELO
template <class T>
struct Insert {

    int min_;
    int max_;
    LinkedList<T>* ptr_;

    Insert(int min, int max, LinkedList<T>* ptr) :min_(min), max_(max), ptr_(ptr) {}

    void operator()(int operaciones) {

        for (int i = 0; i < operaciones; i++) {
            cout << "INSERT" << endl;
            ptr_->insert(get_random(min_, max_));
        }
    }
};

template <class T>
struct Remove {

    int min_;
    int max_;
    LinkedList<T>* ptr_;

    Remove(int min, int max, LinkedList<T>* ptr) :min_(min), max_(max), ptr_(ptr) {}

    void operator()(int operaciones) {

        for (int i = 0; i < operaciones; i++) {
            cout << "REMOVE" << endl;
            ptr_->remove(get_random(min_, max_));
        }
    }
};


// NUMERO DE OPERACIONES POR THREAD
int num_operaciones_borrar = 10000;
int num_operaciones_insertar = 10000;

int main()
{
    // PRUEBA CON INSERT Y REMOVE EN PARALELO
    LinkedList<int> l;
    vector<thread>insert_threads;
    vector<thread>remove_threads;
    int nt = thread::hardware_concurrency(); // Obtengo el maximo numero de threads de mi pc

    cout << "--------------------INSERCION Y BORRADO EN PARALELO--------------------" << endl;

    // El rango maximo de numeros aleatorios es el numero de operaciones * nt para asegurarme que se ejecute el remove en la lista
    for (int i = 0; i < nt; i++) {
        insert_threads.emplace_back(thread(Insert<int>(0, num_operaciones_insertar * nt, &l), num_operaciones_insertar));
        remove_threads.emplace_back(thread(Remove<int>(0, num_operaciones_borrar * nt, &l), num_operaciones_borrar));
    }
    for (int i = 0; i < nt; i++) {
        insert_threads[i].join();
        remove_threads[i].join();
    }
    cout << "\n--------------------IMPRESION DE LA LISTA--------------------" << endl;
    l.print();


    return 0;
}