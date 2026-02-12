# C++ Moderno, breves apuntes

## Propiedad (Ownership)
En C++, la propiedad define quién es responsable de gestionar y liberar un recurso.  
Un objeto dueño controla su ciclo de vida (std::string, std::vector, std::unique_ptr), mientras que un observador solo accede sin poseerlo (std::string_view, referencias, punteros raw). También existen dueños compartidos (std::shared_ptr) que liberan el recurso cuando el último deja de usarlo.  
Entender quién posee cada recurso es esencial para evitar errores como fugas de memoria, dobles liberaciones o accesos inválidos.

## Relación entre Propiedad y RAII
RAII (Resource Acquisition Is Initialization) es el mecanismo que C++ usa para implementar la propiedad de forma segura. Es sencillo: el objeto que posee un recurso lo adquiere en su constructor y lo libera automáticamente en su destructor. Así, la vida del recurso queda ligada a la vida del objeto dueño. Este patrón elimina la necesidad de liberar manualmente memoria o recursos y previene errores comunes como fugas o dobles liberaciones, haciendo que la gestión de propiedad sea explícita y confiable.

## Smart Pointers en C++ — Resumen
Los smart pointers son objetos que gestionan automáticamente memoria dinámica siguiendo el principio RAII. Reemplazan el uso manual de new y delete, evitando fugas y errores de liberación.

### std::unique_ptr → Propiedad exclusiva. Un solo dueño. No copiable, solo movible.
- **Con auto:** `auto p1 = std::make_unique<int>(10);`
- **Sin auto:** `std::unique_ptr<int> p1 = std::make_unique<int>(10);`
- **Mover propiedad:** `std::unique_ptr<int> p2 = std::move(p1);  // p1 queda nullptr`

### std::shared_ptr → Propiedad compartida. Contador de referencias. Libera cuando el último dueño desaparece.
- **Con auto:** `auto p1 = std::make_shared<int>(10);`
- **Sin auto:** `std::shared_ptr<int> p1 = std::make_shared<int>(10);`
- **Compartir:** `std::shared_ptr<int> p2 = p1;  // contador = 2`

### std::weak_ptr → Observador sin propiedad. No incrementa contador. Evita ciclos.
- **Sin auto:** `std::weak_ptr<int> w = p1;`
- **Usar:** `std::shared_ptr<int> sp = w.lock();  // nullptr si expiró`

## Lifetime
El lifetime de un objeto es el periodo durante el cual la memoria del objeto está válida y se puede acceder a él sin provocar comportamiento indefinido.
### Esquema memoria-lifetime
```
.text    -> Código ejecutable
          Lifetime: todo el programa (instrucciones)

.rodata  -> Literales y consts (solo lectura)
          Lifetime: todo el programa
          Ej: "Hola Mundo", const int x = 42;

.data    -> Global/static inicializados (lectura/escritura)
          Lifetime: todo el programa
          Ej: int g = 5; static int s = 10;

.bss     -> Global/static sin inicializar (lectura/escritura, inicializados a 0)
          Lifetime: todo el programa
          Ej: int g2; static int s2;

Heap     -> Memoria dinámica (new / malloc)
          Lifetime: hasta delete / free
          Ej: int* p = new int(42);

Stack    -> Variables locales
          Lifetime: desde que se declara hasta salir del bloque {}
          Ej: int x = 10;
```

## std::string_view
Vista de solo lectura a una cadena de caracteres existente. Internamente es solo un puntero al inicio y un tamaño, no copia ni gestiona memoria.

### Propiedad
`string_view` no posee los datos que observa. Es un observador puro, similar a un puntero raw pero más seguro porque incluye el tamaño. El dato original debe seguir vivo mientras el `string_view` exista.

### Lifetime
El mayor peligro de `string_view` es que el dato subyacente muera antes que la vista:
```cpp
// ✅ OK - literal vive para siempre, esta guardado en .rodata
std::string_view sv = "hello";

// ✅ OK - string vive más que string_view
std::string s = "hello";
std::string_view sv = s;

// ❌ El string temporal muere, sv apunta a basura
std::string_view sv = std::string("hello");

// ❌ La función retorna vista a variable local que muere
std::string_view bad() {
    std::string s = "hello";
    return s;
}
```
**Regla:** Nunca almacenar `string_view` a datos temporales. Usarlo principalmente como parámetro de función donde el caller garantiza el lifetime.

## Compile-time vs Runtime

**Compile-time:** Ocurre cuando el compilador genera el ejecutable.

**Runtime:** Ocurre cuando el programa se ejecuta.

### Ventajas de compile-time

| Aspecto | Compile-time | Runtime |
|---------|--------------|---------|
| Errores | Al compilar | Crash en producción |
| Velocidad | Costo cero | Se calcula cada ejecución |
| Binario | Valor embebido | Código para calcularlo |

### constexpr

Indica que algo puede evaluarse en compile-time:
```cpp
constexpr int x = 5 * 5;                       // Compile-time garantizado
constexpr int square(int n) { return n * n; }  // Compile-time si args son constantes

constexpr int a = square(5);    // ✅ Compile-time
int b = square(variable);       // ✅ Runtime (también válido)
```

**Regla:** Variable `constexpr` = siempre compile-time. Función `constexpr` = depende de los argumentos.