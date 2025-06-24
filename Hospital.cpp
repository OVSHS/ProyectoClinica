#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <regex>

using namespace std;

// Variables globales
static int siguienteIdPaciente = 1;
static int siguienteIdMedico = 1;
static bool programaActivo = true;
mutex mutexAccesoDatos;

// Funciones de validacion
bool validarFecha(const string &fecha)
{
    regex patron("^\\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])$");
    return regex_match(fecha, patron);
}

bool validarTelefono(const string &telefono)
{
    regex patron("^[0-9]{8,15}$");
    return regex_match(telefono, patron);
}

bool validarCorreo(const string &correo)
{
    regex patron("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return regex_match(correo, patron);
}

bool validarGenero(const string &genero)
{
    vector<string> generosValidos = {"Masculino", "Femenino"};
    return find(generosValidos.begin(), generosValidos.end(), genero) != generosValidos.end();
}

bool validarIdentidad(const string &identidad)
{

    return !identidad.empty() && identidad.length() >= 6;
}

class Consulta
{
public:
    string fecha;
    string motivo;
    string diagnostico;
    string tratamiento;
    string notas;
    int idMedico;

    Consulta() = default;

    Consulta(const string &fechaConsulta, const string &motivoConsulta,
             const string &diagnosticoConsulta, const string &tratamientoConsulta,
             const string &notasConsulta, int idMedicoConsulta) : fecha(fechaConsulta), motivo(motivoConsulta), diagnostico(diagnosticoConsulta),
                                                                  tratamiento(tratamientoConsulta), notas(notasConsulta), idMedico(idMedicoConsulta) {}

    // Convierte la consulta a texto para almacenamiento
    string aTexto() const
    {
        return fecha + '|' + to_string(idMedico) + '|' + motivo + '|' +
               diagnostico + '|' + tratamiento + '|' + notas;
    }

    // Crea una consulta desde una linea de texto
    static Consulta desdeTexto(const string &linea)
    {
        stringstream flujo(linea);
        string fechaTxt, motivoTxt, diagnosticoTxt, tratamientoTxt, notasTxt, temp;

        getline(flujo, fechaTxt, '|');
        getline(flujo, temp, '|');
        int idMedicoTxt = stoi(temp);
        getline(flujo, motivoTxt, '|');
        getline(flujo, diagnosticoTxt, '|');
        getline(flujo, tratamientoTxt, '|');
        getline(flujo, notasTxt, '|');

        return Consulta(fechaTxt, motivoTxt, diagnosticoTxt,
                        tratamientoTxt, notasTxt, idMedicoTxt);
    }
};

class ExpedienteClinico
{
    vector<Consulta> consultas;

public:
    void agregarConsulta(const Consulta &nuevaConsulta)
    {
        consultas.insert(consultas.begin(), nuevaConsulta);
    }

    const vector<Consulta> &obtenerConsultas() const
    {
        return consultas;
    }

    string aTexto() const
    {
        string resultado;
        for (const auto &consulta : consultas)
        {
            resultado += consulta.aTexto() + "\n";
        }
        return resultado;
    }

    void cargarDesdeFlujo(istream &flujo)
    {
        string linea;
        while (getline(flujo, linea) && linea != "$")
        {
            if (!linea.empty())
            {
                consultas.push_back(Consulta::desdeTexto(linea));
            }
        }
    }
};

class Paciente
{
    int idPaciente;
    string nombreCompleto;
    string fechaNacimiento;
    string direccion;
    string identidad;
    string telefono;
    string correo;
    string genero;
    ExpedienteClinico expediente;

public:
    Paciente() = default;

    Paciente(const string &nombre, const string &fechaNac, const string &dir,
             const string &id, const string &tel, const string &email,
             const string &gen) : idPaciente(siguienteIdPaciente++), nombreCompleto(nombre), fechaNacimiento(fechaNac),
                                  direccion(dir), identidad(id), telefono(tel), correo(email), genero(gen) {}

    int obtenerId() const { return idPaciente; }
    const string &obtenerNombre() const { return nombreCompleto; }
    ExpedienteClinico &obtenerExpediente() { return expediente; }
    const ExpedienteClinico &obtenerExpediente() const { return expediente; }

    string aTexto() const
    {
        return to_string(idPaciente) + '|' + nombreCompleto + '|' + fechaNacimiento + '|' +
               direccion + '|' + identidad + '|' + telefono + '|' + correo + '|' + genero;
    }

    static Paciente desdeTexto(const string &linea)
    {
        stringstream flujo(linea);
        string temp, nombre, fechaNac, dir, id, tel, email, gen;

        getline(flujo, temp, '|');
        int idTxt = stoi(temp);
        getline(flujo, nombre, '|');
        getline(flujo, fechaNac, '|');
        getline(flujo, dir, '|');
        getline(flujo, id, '|');
        getline(flujo, tel, '|');
        getline(flujo, email, '|');
        getline(flujo, gen, '|');

        Paciente tempPaciente(nombre, fechaNac, dir, id, tel, email, gen);
        tempPaciente.idPaciente = idTxt;
        return tempPaciente;
    }
};

class Medico
{
    int idMedico;
    string nombreCompleto;
    string especialidad;
    string numeroColegiacion;
    bool disponible;

public:
    Medico() = default;

    Medico(const string &nombre, const string &esp, const string &numColegiado) : idMedico(siguienteIdMedico++), nombreCompleto(nombre),
                                                                                  especialidad(esp), numeroColegiacion(numColegiado), disponible(true) {}

    int obtenerId() const
    {
        return idMedico;
    }
    const string &obtenerNombre() const
    {
        return nombreCompleto;
    }
    const string &obtenerEspecialidad() const
    {
        return especialidad;
    }
    bool estaDisponible() const
    {
        return disponible;
    }

    void cambiarDisponibilidad(bool disp)
    {
        disponible = disp;
    }

    string aTexto() const
    {
        return to_string(idMedico) + '|' + nombreCompleto + '|' + especialidad + '|' +
               numeroColegiacion + '|' + (disponible ? "1" : "0");
    }

    static Medico desdeTexto(const string &linea)
    {
        stringstream flujo(linea);
        string temp, nombre, esp, numColegiado, disp;

        getline(flujo, temp, '|');
        int idTxt = stoi(temp);
        getline(flujo, nombre, '|');
        getline(flujo, esp, '|');
        getline(flujo, numColegiado, '|');
        getline(flujo, disp, '|');

        Medico tempMedico(nombre, esp, numColegiado);
        tempMedico.idMedico = idTxt;
        tempMedico.disponible = (disp == "1");
        return tempMedico;
    }
};

struct ColaEspecialidad
{
    string nombreEspecialidad;
    vector<int> idsPacientes;
};

class SistemaHospital
{
    vector<Paciente> pacientes;
    vector<Medico> medicos;
    vector<ColaEspecialidad> colasEspera;
    string contrasenaAdmin;

    // Metodos auxiliares
    static void solicitarCampo(const string &etiqueta, string &valor, bool obligatorio = true)
    {
        do
        {
            cout << etiqueta << ": ";
            getline(cin, valor);

            if (obligatorio && valor.empty())
            {
                cout << "Este campo es obligatorio. Por favor ingrese un valor.\n";
            }
        } while (obligatorio && valor.empty());
    }

    static string obtenerFechaActual()
    {
        time_t ahora = time(nullptr);
        tm *tiempoLocal = localtime(&ahora);
        char buffer[11];
        strftime(buffer, 11, "%Y-%m-%d", tiempoLocal);
        return buffer;
    }

    Paciente *buscarPacientePorId(int idPaciente)
    {
        for (auto &paciente : pacientes)
        {
            if (paciente.obtenerId() == idPaciente)
            {
                return &paciente;
            }
        }
        return nullptr;
    }

    Medico *buscarMedicoPorId(int idMedico)
    {
        for (auto &medico : medicos)
        {
            if (medico.obtenerId() == idMedico)
            {
                return &medico;
            }
        }
        return nullptr;
    }

    ColaEspecialidad *buscarColaPorEspecialidad(const string &especialidad)
    {
        for (auto &cola : colasEspera)
        {
            if (cola.nombreEspecialidad == especialidad)
            {
                return &cola;
            }
        }
        return nullptr;
    }

    string obtenerNombreMedicoPorId(int idMedico)
    {
        Medico *medico = buscarMedicoPorId(idMedico);
        return medico ? medico->obtenerNombre() : "Desconocido";
    }

public:
    // Metodos utilitarios
    static int leerId(const string &mensaje)
    {
        int valor;
        while (true)
        {
            cout << mensaje << ": ";
            if (cin >> valor)
            {
                cin.ignore();
                return valor;
            }
            else
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Entrada invalida. Por favor ingrese un numero.\n";
            }
        }
    }

    bool existePaciente(int idPaciente)
    {
        lock_guard<mutex> bloqueo(mutexAccesoDatos);
        return buscarPacientePorId(idPaciente) != nullptr;
    }

    bool existeMedico(int idMedico)
    {
        lock_guard<mutex> bloqueo(mutexAccesoDatos);
        return buscarMedicoPorId(idMedico) != nullptr;
    }

    bool tieneAdmin() const
    {
        return !contrasenaAdmin.empty();
    }

    // Metodos de creacion
    int crearPaciente()
    {
        string nombre, fechaNac, dir, id, tel, email, gen;

        solicitarCampo("Nombre completo", nombre);

        do
        {
            solicitarCampo("Fecha de nacimiento (AAAA-MM-DD)", fechaNac);
            if (!validarFecha(fechaNac))
            {
                cout << "Formato de fecha invalido. Use AAAA-MM-DD.\n";
            }
        } while (!validarFecha(fechaNac));

        solicitarCampo("Direccion", dir);

        do
        {
            solicitarCampo("Numero de identidad", id);
            if (!validarIdentidad(id))
            {
                cout << "Numero de identidad invalido. Debe tener al menos 6 caracteres.\n";
            }
        } while (!validarIdentidad(id));

        do
        {
            solicitarCampo("Telefono", tel);
            if (!validarTelefono(tel))
            {
                cout << "Telefono invalido. Debe contener solo numeros (8-15 digitos).\n";
            }
        } while (!validarTelefono(tel));

        do
        {
            solicitarCampo("Correo electronico", email);
            if (!validarCorreo(email))
            {
                cout << "Correo electronico invalido.\n";
            }
        } while (!validarCorreo(email));

        do
        {
            solicitarCampo("Genero (Masculino/Femenino)", gen);
            if (!validarGenero(gen))
            {
                cout << "Genero invalido. Opciones validas: Masculino, Femenino.\n";
            }
        } while (!validarGenero(gen));

        lock_guard<mutex> bloqueo(mutexAccesoDatos);
        pacientes.emplace_back(nombre, fechaNac, dir, id, tel, email, gen);
        int nuevoId = pacientes.back().obtenerId();

        cout << "Paciente creado con ID " << nuevoId << "\n";
        return nuevoId;
    }

    int crearMedico()
    {
        string nombre, especialidad, numColegiado;

        solicitarCampo("Nombre completo", nombre);
        solicitarCampo("Especialidad", especialidad);

        do
        {
            solicitarCampo("Numero de colegiado", numColegiado);
            if (numColegiado.empty())
            {
                cout << "El numero de colegiado es obligatorio.\n";
            }
        } while (numColegiado.empty());

        lock_guard<mutex> bloqueo(mutexAccesoDatos);
        medicos.emplace_back(nombre, especialidad, numColegiado);
        int nuevoId = medicos.back().obtenerId();

        cout << "Medico creado con ID " << nuevoId << "\n";
        return nuevoId;
    }

    void crearAdmin()
    {
        if (tieneAdmin())
        {
            cout << "Ya existe un administrador\n";
            return;
        }

        string contrasena, confirmarContrasena;
        do
        {
            solicitarCampo("Contraseña de administrador", contrasena);
            solicitarCampo("Confirmar contraseña", confirmarContrasena);

            if (contrasena != confirmarContrasena)
            {
                cout << "Las contraseñas no coinciden\n";
            }
            else if (contrasena.empty())
            {
                cout << "La contraseña no puede estar vacia\n";
            }
        } while (contrasena.empty() || contrasena != confirmarContrasena);

        contrasenaAdmin = contrasena;
        cout << "Cuenta de administrador creada\n";
    }

    bool validarAdmin()
    {
        string contrasena;
        solicitarCampo("Contraseña", contrasena);

        if (contrasena == contrasenaAdmin)
        {
            return true;
        }

        cout << "Contraseña incorrecta\n";
        return false;
    }

    // Operaciones de paciente
    void pacienteTomarTurno(int idPacienteSesion)
    {
        // Verificar si el paciente ya tiene un turno
        {
            lock_guard<mutex> bloqueo(mutexAccesoDatos);
            for (const auto &cola : colasEspera)
            {
                auto posicion = find(cola.idsPacientes.begin(), cola.idsPacientes.end(), idPacienteSesion);
                if (posicion != cola.idsPacientes.end())
                {
                    cout << "Ya tiene un turno pendiente. Espere a ser atendido.\n";
                    return;
                }
            }
        }

        // Obtener medicos disponibles
        vector<Medico *> medicosDisponibles;
        {
            lock_guard<mutex> bloqueo(mutexAccesoDatos);
            for (auto &medico : medicos)
            {
                if (medico.estaDisponible())
                {
                    medicosDisponibles.push_back(&medico);
                }
            }
        }

        if (medicosDisponibles.empty())
        {
            cout << "No hay medicos disponibles en este momento\n";
            return;
        }

        // Mostrar menu de medicos disponibles
        cout << "\nMedicos disponibles:\n";
        for (size_t i = 0; i < medicosDisponibles.size(); ++i)
        {
            cout << i + 1 << ". " << medicosDisponibles[i]->obtenerNombre()
                 << " - " << medicosDisponibles[i]->obtenerEspecialidad() << '\n';
        }
        cout << "0. Regresar\n";

        int opcionSeleccionada;
        while (true)
        {
            cout << "Seleccione una opcion: ";
            if (cin >> opcionSeleccionada)
            {
                cin.ignore();
                break;
            }
            else
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Entrada invalida. Por favor ingrese un numero.\n";
            }
        }

        if (opcionSeleccionada == 0)
            return;
        if (opcionSeleccionada < 1 || opcionSeleccionada > static_cast<int>(medicosDisponibles.size()))
        {
            cout << "Opcion invalida\n";
            return;
        }

        Medico *medicoSeleccionado = medicosDisponibles[opcionSeleccionada - 1];

        // Agregar paciente a la cola
        {
            lock_guard<mutex> bloqueo(mutexAccesoDatos);
            string especialidadMedico = medicoSeleccionado->obtenerEspecialidad();
            ColaEspecialidad *cola = buscarColaPorEspecialidad(especialidadMedico);

            if (!cola)
            {
                colasEspera.push_back({especialidadMedico, {}});
                cola = &colasEspera.back();
            }

            cola->idsPacientes.push_back(idPacienteSesion);
        }

        cout << "Turno tomado con " << medicoSeleccionado->obtenerNombre()
             << " (Especialidad: " << medicoSeleccionado->obtenerEspecialidad() << ")\n";
    }

    // Operaciones de medico
    void atenderPaciente()
    {
        int idMedico = leerId("Confirme su ID de medico");
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        Medico *medico = buscarMedicoPorId(idMedico);
        if (!medico)
        {
            cout << "ID incorrecto\n";
            return;
        }

        string especialidad = medico->obtenerEspecialidad();
        ColaEspecialidad *cola = buscarColaPorEspecialidad(especialidad);

        if (!cola || cola->idsPacientes.empty())
        {
            cout << "No hay pacientes en espera\n";
            return;
        }

        int idPaciente = cola->idsPacientes.front();
        cola->idsPacientes.erase(cola->idsPacientes.begin());

        Paciente *paciente = buscarPacientePorId(idPaciente);
        if (!paciente)
        {
            cout << "Paciente no encontrado\n";
            return;
        }

        medico->cambiarDisponibilidad(false);

        string motivo, diagnostico, tratamiento, notas;
        string fechaActual = obtenerFechaActual();

        solicitarCampo("Motivo de la consulta", motivo);
        solicitarCampo("Diagnostico", diagnostico);
        solicitarCampo("Tratamiento", tratamiento);
        solicitarCampo("Notas adicionales", notas, false); // No obligatorio

        paciente->obtenerExpediente().agregarConsulta(
            Consulta(fechaActual, motivo, diagnostico, tratamiento, notas, medico->obtenerId()));

        medico->cambiarDisponibilidad(true);
        cout << "Consulta registrada exitosamente\n";
    }

    void verExpediente()
    {
        int idPaciente = leerId("ID del paciente");
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        Paciente *paciente = buscarPacientePorId(idPaciente);
        if (!paciente)
        {
            cout << "Paciente no encontrado\n";
            return;
        }

        const auto &consultas = paciente->obtenerExpediente().obtenerConsultas();
        if (consultas.empty())
        {
            cout << "No hay registros medicos\n";
            return;
        }

        cout << "----------------------------------------\n";

        cout << "Expediente de " << paciente->obtenerNombre()
             << " (ID " << paciente->obtenerId() << ")\n\n";

        for (const auto &consulta : consultas)
        {
            cout << "Fecha: " << consulta.fecha
                 << " | Medico: " << obtenerNombreMedicoPorId(consulta.idMedico)
                 << "\nMotivo: " << consulta.motivo
                 << "\nDiagnostico: " << consulta.diagnostico << "\n"
                 << "----------------------------------------\n";
        }
    }

    // Operaciones de administrador
    void adminEliminarMedico()
    {
        int idMedico = leerId("ID del medico a eliminar");
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        auto it = remove_if(medicos.begin(), medicos.end(),
                            [&](const Medico &medico)
                            {
                                return medico.obtenerId() == idMedico;
                            });

        if (it != medicos.end())
        {
            medicos.erase(it, medicos.end());
            cout << "Medico eliminado exitosamente\n";
        }
        else
        {
            cout << "Medico no encontrado\n";
        }
    }

    void adminListarMedicos()
    {
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        cout << "\nMedicos registrados:\n";
        for (const auto &medico : medicos)
        {
            cout << "ID: " << medico.obtenerId()
                 << " | Nombre: " << medico.obtenerNombre()
                 << " | Especialidad: " << medico.obtenerEspecialidad()
                 << " | Estado: " << (medico.estaDisponible() ? "Disponible" : "Ocupado") << "\n";
        }
    }

    void adminVerColaMedico()
    {
        int idMedico = leerId("ID del medico");
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        Medico *medico = buscarMedicoPorId(idMedico);
        if (!medico)
        {
            cout << "Medico no encontrado\n";
            return;
        }

        ColaEspecialidad *cola = buscarColaPorEspecialidad(medico->obtenerEspecialidad());
        if (!cola || cola->idsPacientes.empty())
        {
            cout << "La cola esta vacia\n";
            return;
        }

        cout << "Cola de " << medico->obtenerNombre()
             << " (" << medico->obtenerEspecialidad() << ")\n";

        const int anchoPosicion = 10;
        cout << left << setw(anchoPosicion) << "Posicion" << "Nombre del paciente\n";

        for (size_t i = 0; i < cola->idsPacientes.size(); ++i)
        {
            int idPaciente = cola->idsPacientes[i];
            Paciente *paciente = buscarPacientePorId(idPaciente);
            string nombrePaciente = paciente ? paciente->obtenerNombre() : "Desconocido";

            cout << left << setw(anchoPosicion) << i + 1 << nombrePaciente << '\n';
        }
    }

    void adminEliminarDeCola()
    {
        int idMedico = leerId("ID del medico");
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        Medico *medico = buscarMedicoPorId(idMedico);
        if (!medico)
        {
            cout << "Medico no encontrado\n";
            return;
        }

        ColaEspecialidad *cola = buscarColaPorEspecialidad(medico->obtenerEspecialidad());
        if (!cola || cola->idsPacientes.empty())
        {
            cout << "La cola esta vacia\n";
            return;
        }

        cout << "Pacientes en cola:\n";
        for (size_t i = 0; i < cola->idsPacientes.size(); ++i)
        {
            Paciente *paciente = buscarPacientePorId(cola->idsPacientes[i]);
            cout << i + 1 << ". " << (paciente ? paciente->obtenerNombre() : "Desconocido") << "\n";
        }

        int posicion = leerId("Posicion a eliminar");
        if (posicion < 1 || posicion > static_cast<int>(cola->idsPacientes.size()))
        {
            cout << "Posicion invalida\n";
            return;
        }

        cola->idsPacientes.erase(cola->idsPacientes.begin() + posicion - 1);
        cout << "Turno eliminado de la cola\n";
    }

    // Estadísticas
    void generarReporte()
    {
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        size_t totalConsultas = 0;
        for (const auto &paciente : pacientes)
        {
            totalConsultas += paciente.obtenerExpediente().obtenerConsultas().size();
        }

        cout << "Total de consultas: " << totalConsultas << "\n";
    }

    // Persistencia de datos
    void guardarDatos()
    {
        lock_guard<mutex> bloqueo(mutexAccesoDatos);

        ofstream archivoPacientes("Pacientes.txt"),
            archivoMedicos("Medicos.txt"),
            archivoExpedientes("Expedientes.txt"),
            archivoAdmin("Admin.txt");

        for (const auto &paciente : pacientes)
        {
            archivoPacientes << paciente.aTexto() << "\n";
        }

        for (const auto &medico : medicos)
        {
            archivoMedicos << medico.aTexto() << "\n";
        }

        for (const auto &paciente : pacientes)
        {
            archivoExpedientes << "#" << paciente.obtenerId() << "\n"
                               << paciente.obtenerExpediente().aTexto() << "$\n";
        }

        archivoAdmin << contrasenaAdmin << "\n";
    }

    void cargarDatos()
    {
        int maximoIdPaciente = 0;
        int maximoIdMedico = 0;

        ifstream archivoPacientes("Pacientes.txt");
        string lineaLeida;
        while (getline(archivoPacientes, lineaLeida) && !lineaLeida.empty())
        {
            Paciente pacienteLeido = Paciente::desdeTexto(lineaLeida);
            maximoIdPaciente = max(maximoIdPaciente, pacienteLeido.obtenerId());
            pacientes.push_back(move(pacienteLeido));
        }

        ifstream archivoMedicos("Medicos.txt");
        while (getline(archivoMedicos, lineaLeida) && !lineaLeida.empty())
        {
            Medico medicoLeido = Medico::desdeTexto(lineaLeida);
            maximoIdMedico = max(maximoIdMedico, medicoLeido.obtenerId());
            medicos.push_back(move(medicoLeido));
        }

        ifstream archivoExpedientes("Expedientes.txt");
        while (getline(archivoExpedientes, lineaLeida))
        {
            if (!lineaLeida.empty() && lineaLeida[0] == '#')
            {
                int idPacienteExp = stoi(lineaLeida.substr(1));
                Paciente *pacienteDestino = buscarPacientePorId(idPacienteExp);

                stringstream bufferFlujo;
                string lineaExp;
                while (getline(archivoExpedientes, lineaExp) && lineaExp != "$")
                    bufferFlujo << lineaExp << '\n';

                if (pacienteDestino)
                    pacienteDestino->obtenerExpediente().cargarDesdeFlujo(bufferFlujo);
            }
        }

        ifstream archivoAdmin("Admin.txt");
        getline(archivoAdmin, contrasenaAdmin);

        siguienteIdPaciente = maximoIdPaciente + 1;
        siguienteIdMedico = maximoIdMedico + 1;
    }
};

// Hilos en segundo plano
void hiloGuardarAutomatico(SistemaHospital *sistema)
{
    while (programaActivo)
    {
        this_thread::sleep_for(chrono::minutes(5));
        sistema->guardarDatos();
    }
}

void hiloReporteAutomatico(SistemaHospital *sistema)
{
    while (programaActivo)
    {
        this_thread::sleep_for(chrono::minutes(2));
    }
}

// Funciones de menu
void mostrarMenuPrincipal()
{
    cout << "\n========================================\n";
    cout << "       SISTEMA HOSPITALARIO\n";
    cout << "========================================\n";
    cout << "1. Paciente\n";
    cout << "2. Medico\n";
    cout << "3. Administrador\n";
    cout << "0. Salir\n";
    cout << "----------------------------------------\n";
    cout << "Seleccione una opcion: ";
}

void mostrarMenuPaciente()
{
    cout << "\n========================================\n";
    cout << "          MENU DE PACIENTE\n";
    cout << "========================================\n";
    cout << "1. Tomar turno\n";
    cout << "0. Cerrar sesion\n";
    cout << "----------------------------------------\n";
    cout << "Seleccione una opcion: ";
}

void mostrarMenuMedico()
{
    cout << "\n========================================\n";
    cout << "          MENU DE MEDICO\n";
    cout << "========================================\n";
    cout << "1. Ver expediente\n";
    cout << "2. Atender paciente\n";
    cout << "3. Generar reporte\n";
    cout << "0. Cerrar sesion\n";
    cout << "----------------------------------------\n";
    cout << "Seleccione una opcion: ";
}

void mostrarMenuAdmin()
{
    cout << "\n========================================\n";
    cout << "       MENU DE ADMINISTRADOR\n";
    cout << "========================================\n";
    cout << "1. Listar medicos\n";
    cout << "2. Eliminar medico\n";
    cout << "3. Ver cola de medico\n";
    cout << "4. Eliminar de cola\n";
    cout << "5. Ver expediente\n";
    cout << "0. Cerrar sesion\n";
    cout << "----------------------------------------\n";
    cout << "Seleccione una opcion: ";
}

// Programa principal
int main()
{
    SistemaHospital sistemaHospital;
    sistemaHospital.cargarDatos();

    thread hiloGuardar(hiloGuardarAutomatico, &sistemaHospital);
    thread hiloReporte(hiloReporteAutomatico, &sistemaHospital);

    bool ejecutando = true;
    while (ejecutando)
    {
        mostrarMenuPrincipal();

        int opcionRol;
        while (true)
        {
            if (cin >> opcionRol)
            {
                cin.ignore();
                break;
            }
            else
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Entrada invalida. Por favor ingrese un numero.\n";
                mostrarMenuPrincipal();
            }
        }

        if (opcionRol == 0)
        {
            ejecutando = false;
            break;
        }

        // Flujo de paciente
        if (opcionRol == 1)
        {
            bool enMenuPaciente = true;
            while (enMenuPaciente)
            {
                int opcionPaciente;
                cout << "\n========================================\n";
                cout << "       OPCIONES DE PACIENTE\n";
                cout << "========================================\n";
                cout << "1. Iniciar sesion\n";
                cout << "2. Registrarse\n";
                cout << "0. Regresar\n";
                cout << "----------------------------------------\n";
                cout << "Seleccione una opcion: ";

                while (true)
                {
                    if (cin >> opcionPaciente)
                    {
                        cin.ignore();
                        break;
                    }
                    else
                    {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Entrada invalida. Por favor ingrese un numero.\n";
                        cout << "Seleccione una opcion: ";
                    }
                }

                if (opcionPaciente == 0)
                    break;

                if (opcionPaciente == 2)
                {
                    sistemaHospital.crearPaciente();
                }
                else if (opcionPaciente == 1)
                {
                    int idPaciente = SistemaHospital::leerId("ID de paciente");
                    if (!sistemaHospital.existePaciente(idPaciente))
                    {
                        cout << "ID invalido\n";
                        continue;
                    }

                    int opcionMenuPaciente = 1;
                    while (opcionMenuPaciente != 0)
                    {
                        mostrarMenuPaciente();
                        while (true)
                        {
                            if (cin >> opcionMenuPaciente)
                            {
                                cin.ignore();
                                break;
                            }
                            else
                            {
                                cin.clear();
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                cout << "Entrada invalida. Por favor ingrese un numero.\n";
                                mostrarMenuPaciente();
                            }
                        }

                        if (opcionMenuPaciente == 1)
                        {
                            sistemaHospital.pacienteTomarTurno(idPaciente);
                        }
                    }
                    enMenuPaciente = false;
                }
                else
                {
                    cout << "Opcion invalida\n";
                }
            }
        }

        // Flujo de medico
        if (opcionRol == 2)
        {
            bool enMenuMedico = true;
            while (enMenuMedico)
            {
                int opcionMedico;
                cout << "\n========================================\n";
                cout << "       OPCIONES DE MEDICO\n";
                cout << "========================================\n";
                cout << "1. Iniciar sesion\n";
                cout << "2. Registrarse\n";
                cout << "0. Regresar\n";
                cout << "----------------------------------------\n";
                cout << "Seleccione una opcion: ";

                while (true)
                {
                    if (cin >> opcionMedico)
                    {
                        cin.ignore();
                        break;
                    }
                    else
                    {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Entrada invalida. Por favor ingrese un numero.\n";
                        cout << "Seleccione una opcion: ";
                    }
                }

                if (opcionMedico == 0)
                    break;

                if (opcionMedico == 2)
                {
                    sistemaHospital.crearMedico();
                }
                else if (opcionMedico == 1)
                {
                    int idMedico = SistemaHospital::leerId("ID de medico");
                    if (!sistemaHospital.existeMedico(idMedico))
                    {
                        cout << "ID invalido\n";
                        continue;
                    }

                    int opcionMenuMedico = 1;
                    while (opcionMenuMedico != 0)
                    {
                        mostrarMenuMedico();
                        while (true)
                        {
                            if (cin >> opcionMenuMedico)
                            {
                                cin.ignore();
                                break;
                            }
                            else
                            {
                                cin.clear();
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                cout << "Entrada invalida. Por favor ingrese un numero.\n";
                                mostrarMenuMedico();
                            }
                        }

                        if (opcionMenuMedico == 1)
                        {
                            sistemaHospital.verExpediente();
                        }
                        else if (opcionMenuMedico == 2)
                        {
                            sistemaHospital.atenderPaciente();
                        }
                        else if (opcionMenuMedico == 3)
                        {
                            sistemaHospital.generarReporte();
                        }
                    }
                    enMenuMedico = false;
                }
                else
                {
                    cout << "Opcion invalida\n";
                }
            }
        }

        // Flujo de administrador
        if (opcionRol == 3)
        {
            bool enMenuAdmin = true;
            while (enMenuAdmin)
            {
                cout << "\n========================================\n";
                cout << "     OPCIONES DE ADMINISTRADOR\n";
                cout << "========================================\n";
                cout << "1. Iniciar sesion\n";
                cout << "2. Crear cuenta de administrador\n";
                cout << "0. Regresar\n";
                cout << "----------------------------------------\n";
                cout << "Seleccione una opcion: ";

                int opcionAdmin;
                while (true)
                {
                    if (cin >> opcionAdmin)
                    {
                        cin.ignore();
                        break;
                    }
                    else
                    {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Entrada invalida. Por favor ingrese un numero.\n";
                        cout << "Seleccione una opcion: ";
                    }
                }

                if (opcionAdmin == 0)
                    break;

                if (opcionAdmin == 2)
                {
                    sistemaHospital.crearAdmin();
                }
                else if (opcionAdmin == 1)
                {
                    if (!sistemaHospital.tieneAdmin())
                    {
                        cout << "No existe una cuenta de administrador\n";
                        continue;
                    }

                    if (!sistemaHospital.validarAdmin())
                    {
                        continue;
                    }

                    int opcionMenuAdmin = 1;
                    while (opcionMenuAdmin != 0)
                    {
                        mostrarMenuAdmin();
                        while (true)
                        {
                            if (cin >> opcionMenuAdmin)
                            {
                                cin.ignore();
                                break;
                            }
                            else
                            {
                                cin.clear();
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                cout << "Entrada invalida. Por favor ingrese un numero.\n";
                                mostrarMenuAdmin();
                            }
                        }

                        if (opcionMenuAdmin == 1)
                        {
                            sistemaHospital.adminListarMedicos();
                        }
                        else if (opcionMenuAdmin == 2)
                        {
                            sistemaHospital.adminEliminarMedico();
                        }
                        else if (opcionMenuAdmin == 3)
                        {
                            sistemaHospital.adminVerColaMedico();
                        }
                        else if (opcionMenuAdmin == 4)
                        {
                            sistemaHospital.adminEliminarDeCola();
                        }
                        else if (opcionMenuAdmin == 5)
                        {
                            sistemaHospital.verExpediente();
                        }
                    }
                    enMenuAdmin = false;
                }
                else
                {
                    cout << "Opcion invalida\n";
                }
            }
        }
    }

    programaActivo = false;
    hiloGuardar.join();
    hiloReporte.join();
    sistemaHospital.guardarDatos();

    return 0;
}