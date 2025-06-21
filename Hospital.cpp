/*****************************************************
 Sistema Control Filas y Expedientes
 Archivo  Hospital.cpp
 Compilar g++ -std=c++20 Hospital.cpp -lpthread
*****************************************************/
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
#include <limits>

using namespace std;

/* ===================== variables globales ===================== */
static int SiguienteIdPaciente = 1;
static int SiguienteIdMedico   = 1;
static bool ProgramaActivoFlag = true;
mutex AccesoDatosMutex;

/* ===================== clase Consulta ===================== */
class Consulta{
public:
    string Fecha, Motivo, Diagnostico, Tratamiento, Notas;
    int IdMedico;
    Consulta() = default;
    Consulta(const string& FechaCons,const string& MotivoCons,const string& Dx,
             const string& Tx,const string& Not,const int IdMedCons) :
        Fecha(FechaCons),Motivo(MotivoCons),Diagnostico(Dx),
        Tratamiento(Tx),Notas(Not),IdMedico(IdMedCons) {}

    string ATexto() const {
        return Fecha+'|'+to_string(IdMedico)+'|'+Motivo+'|'+Diagnostico+'|'+Tratamiento+'|'+Notas;
    }
    static Consulta DesdeTexto(const string& Linea){
        stringstream Flujo(Linea);
        string F,M,D,T,N,Aux;
        getline(Flujo,F,'|'); getline(Flujo,Aux,'|'); int IdM = stoi(Aux);
        getline(Flujo,M,'|'); getline(Flujo,D,'|'); getline(Flujo,T,'|'); getline(Flujo,N,'|');
        return Consulta(F,M,D,T,N,IdM);
    }
};

/* ===================== clase ExpedienteClinico ===================== */
class ExpedienteClinico{
    vector<Consulta> ListaConsultas;
public:
    void Agregar(const Consulta& Nueva){ ListaConsultas.insert(ListaConsultas.begin(),Nueva); }
    const vector<Consulta>& Consultas() const{ return ListaConsultas; }
    string ATexto() const{
        string Resultado;
        for(const auto& C:ListaConsultas) Resultado+=C.ATexto()+"\n";
        return Resultado;
    }
    void DesdeStream(istream& Flujo){
        string Linea;
        while(getline(Flujo,Linea) && Linea!="$")
            if(!Linea.empty()) ListaConsultas.push_back(Consulta::DesdeTexto(Linea));
    }
};

/* ===================== clase Paciente ===================== */
class Paciente{
    int IdPaciente;
    string NombreCompleto, FechaNacimiento, Direccion, Identidad, Telefono, Correo, Genero;
    ExpedienteClinico Expediente;
public:
    Paciente() = default;
    Paciente(const string& Nombre,const string& FechaNac,const string& DireccionTxt,
             const string& Ident,const string& Tel,const string& CorreoTxt,const string& Gen) :
        IdPaciente(SiguienteIdPaciente++),NombreCompleto(Nombre),FechaNacimiento(FechaNac),
        Direccion(DireccionTxt),Identidad(Ident),Telefono(Tel),Correo(CorreoTxt),Genero(Gen) {}

    int Id() const                    { return IdPaciente; }
    const string& Nombre() const      { return NombreCompleto; }
    void CambiarNombre(const string& Nuevo)      { NombreCompleto = Nuevo; }
    void CambiarDireccion(const string& Nuevo)   { Direccion = Nuevo; }
    ExpedienteClinico& Exp()                     { return Expediente; }
    const ExpedienteClinico& Exp() const         { return Expediente; }

    string ATexto() const{
        return to_string(IdPaciente)+'|'+NombreCompleto+'|'+FechaNacimiento+'|'+Direccion+'|'+
               Identidad+'|'+Telefono+'|'+Correo+'|'+Genero;
    }
    static Paciente DesdeTexto(const string& Linea){
        stringstream Flujo(Linea);
        string Aux,N,F,D,I,T,C,G;
        getline(Flujo,Aux,'|'); int IdTxt=stoi(Aux);
        getline(Flujo,N,'|'); getline(Flujo,F,'|'); getline(Flujo,D,'|');
        getline(Flujo,I,'|'); getline(Flujo,T,'|'); getline(Flujo,C,'|'); getline(Flujo,G,'|');
        Paciente Temp(N,F,D,I,T,C,G); Temp.IdPaciente=IdTxt; return Temp;
    }
};

/* ===================== clase Medico ===================== */
class Medico{
    int IdMedico;
    string NombreCompleto, Especialidad, NumeroColegiacion;
    bool DisponibleFlag;
public:
    Medico() = default;
    Medico(const string& Nombre,const string& EspTxt,const string& Numero) :
        IdMedico(SiguienteIdMedico++),NombreCompleto(Nombre),
        Especialidad(EspTxt),NumeroColegiacion(Numero),DisponibleFlag(true) {}

    int Id() const                { return IdMedico; }
    const string& Nombre() const  { return NombreCompleto; }
    const string& Esp() const     { return Especialidad; }
    bool Disponible() const       { return DisponibleFlag; }
    void CambiarEstado(bool Valor){ DisponibleFlag = Valor; }

    string ATexto() const{
        return to_string(IdMedico)+'|'+NombreCompleto+'|'+Especialidad+'|'+NumeroColegiacion+'|'+(DisponibleFlag?"1":"0");
    }
    static Medico DesdeTexto(const string& Linea){
        stringstream Flujo(Linea);
        string Aux,N,E,Num,Dsp;
        getline(Flujo,Aux,'|'); int IdTxt=stoi(Aux);
        getline(Flujo,N,'|'); getline(Flujo,E,'|'); getline(Flujo,Num,'|'); getline(Flujo,Dsp,'|');
        Medico Temp(N,E,Num); Temp.IdMedico=IdTxt; Temp.DisponibleFlag=(Dsp=="1"); return Temp;
    }
};

/* ===================== estructura ColaEspecialidad ===================== */
struct ColaEspecialidad{
    string NombreEspecialidad;
    vector<int> IdsPacienteVector;
};

/* ===================== clase Hospital ===================== */
class Hospital{
    vector<Paciente> ListaPacientes;
    vector<Medico>   ListaMedicos;
    vector<ColaEspecialidad> ListaColas;

/* ---------- utilidades internas ---------- */
    static void Campo(const string& Etiqueta,string& Valor){
        cout<<Etiqueta<<": "; getline(cin,Valor);
    }
    static string FechaHoy(){
        time_t Tiempo=time(nullptr); tm* Local=localtime(&Tiempo); char Buffer[11];
        strftime(Buffer,11,"%Y-%m-%d",Local); return Buffer;
    }
    Paciente* BuscarPaciente(int IdPac){
        for(auto& PacienteActual:ListaPacientes) if(PacienteActual.Id()==IdPac) return &PacienteActual; return nullptr;
    }
    Medico* BuscarMedico(int IdMed){
        for(auto& MedicoActual:ListaMedicos) if(MedicoActual.Id()==IdMed) return &MedicoActual; return nullptr;
    }
    Medico* BuscarMedicoDisponiblePorEsp(const string& Esp){
        for(auto& MedicoActual:ListaMedicos) if(MedicoActual.Esp()==Esp && MedicoActual.Disponible()) return &MedicoActual; return nullptr;
    }
    ColaEspecialidad* BuscarCola(const string& Esp){
        for(auto& ColaActual:ListaColas) if(ColaActual.NombreEspecialidad==Esp) return &ColaActual; return nullptr;
    }
    string NombreMedicoPorId(int IdMed){
        Medico* MedicoTemp=BuscarMedico(IdMed); return MedicoTemp?MedicoTemp->Nombre():string("SinRegistro");
    }

public:
    /* ---------- util publicas ---------- */
    static int LeerId(const string& Etiqueta){ cout<<Etiqueta<<": "; int Valor; cin>>Valor; cin.ignore(); return Valor; }
    bool ExistePaciente(int IdPac){ lock_guard<mutex> Bloqueo(AccesoDatosMutex); return BuscarPaciente(IdPac)!=nullptr; }
    bool ExisteMedico(int IdMed){ lock_guard<mutex> Bloqueo(AccesoDatosMutex); return BuscarMedico(IdMed)!=nullptr; }

    /* ---------- registro ---------- */
    int CrearPaciente(){
        string N,F,D,I,T,C,G;
        Campo("Nombre continuo",N); Campo("Fecha nacimiento",F); Campo("Direccion",D);
        Campo("Identidad",I); Campo("Telefono",T); Campo("Correo",C); Campo("Genero",G);
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        ListaPacientes.emplace_back(N,F,D,I,T,C,G);
        int IdNuevo=ListaPacientes.back().Id();
        cout<<"Paciente creado con ID "<<IdNuevo<<"\n";
        return IdNuevo;
    }
    int CrearMedico(){
        string N,E,Num;
        Campo("Nombre completo",N); Campo("Especialidad",E); Campo("Numero colegiacion",Num);
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        ListaMedicos.emplace_back(N,E,Num);
        int IdNuevo=ListaMedicos.back().Id();
        cout<<"Medico creado con ID "<<IdNuevo<<"\n";
        return IdNuevo;
    }

    /* ---------- paciente opera ---------- */
    void PacienteTomarTurno(int IdPacienteSesion){
        vector<Medico*> ListaDisponibles;
        {
            lock_guard<mutex> Bloqueo(AccesoDatosMutex);
            for(auto& MedicoActual:ListaMedicos)
                if(MedicoActual.Disponible())
                    ListaDisponibles.push_back(&MedicoActual);
        }
        if(ListaDisponibles.empty()){
            cout<<"No hay doctores disponibles\n";
            return;
        }

        cout<<"\nDoctores disponibles\n";
        for(size_t i=0;i<ListaDisponibles.size();++i){
            cout<<i+1<<" "<<ListaDisponibles[i]->Nombre()<<"  "<<ListaDisponibles[i]->Esp()<<"\n";
        }
        cout<<"0 Regresar\nOpcion: ";

        int OpcionSeleccion;
        if(!(cin>>OpcionSeleccion)){
            
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            cout<<"Entrada invalida\n";
            return;
        }
        cin.ignore();
        if(OpcionSeleccion==0) return;
        if(OpcionSeleccion<1 || OpcionSeleccion>static_cast<int>(ListaDisponibles.size())){
            cout<<"Opcion invalida\n";
            return;
        }

        Medico* DoctorElegido=ListaDisponibles[OpcionSeleccion-1];
        {
            lock_guard<mutex> Bloqueo(AccesoDatosMutex);
            string EspecialidadDoctor=DoctorElegido->Esp();
            ColaEspecialidad* Cola=BuscarCola(EspecialidadDoctor);
            if(!Cola){ ListaColas.push_back({EspecialidadDoctor,{}}); Cola=&ListaColas.back(); }
            Cola->IdsPacienteVector.push_back(IdPacienteSesion);
        }
        cout<<"Turno tomado con "<<DoctorElegido->Nombre()<<" especialidad "<<DoctorElegido->Esp()<<"\n";
    }

    /* ---------- doctor opera ---------- */
    void Atender(){
        int IdMedSesion=LeerId("Confirme su ID Doctor");
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        Medico* ObjMed=BuscarMedico(IdMedSesion);
        if(!ObjMed){ cout<<"Id incorrecto\n"; return; }
        string Esp=ObjMed->Esp();
        ColaEspecialidad* Cola=BuscarCola(Esp);
        if(!Cola || Cola->IdsPacienteVector.empty()){ cout<<"Sin pacientes en cola\n"; return; }
        int IdPac=Cola->IdsPacienteVector.front(); Cola->IdsPacienteVector.erase(Cola->IdsPacienteVector.begin());
        Paciente* ObjPac=BuscarPaciente(IdPac);
        if(!ObjPac){ cout<<"Paciente no existe\n"; return; }
        ObjMed->CambiarEstado(false);

        string Mot,Dx,Tx,Nota; string Fh=FechaHoy();
        Campo("Motivo",Mot); Campo("Diagnostico",Dx); Campo("Tratamiento",Tx); Campo("Notas",Nota);
        ObjPac->Exp().Agregar(Consulta(Fh,Mot,Dx,Tx,Nota,ObjMed->Id()));
        ObjMed->CambiarEstado(true);
        cout<<"Consulta registrada\n";
    }
    void VerExpediente(){
        int IdPac=LeerId("ID Paciente");
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        Paciente* ObjPac=BuscarPaciente(IdPac);
        if(!ObjPac){ cout<<"No existe\n"; return; }
        const auto& Vec=ObjPac->Exp().Consultas();
        if(Vec.empty()){ cout<<"Sin registros\n"; return; }
        cout<<"Expediente de "<<ObjPac->Nombre()<<" (ID "<<ObjPac->Id()<<")\n";
        for(const auto& C:Vec){
            cout<<"Fecha "<<C.Fecha<<"  Medico "<<NombreMedicoPorId(C.IdMedico)
                <<"  Motivo "<<C.Motivo<<"  Diagnostico "<<C.Diagnostico<<"\n";
        }
    }
    void Reporte(){
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        size_t Total=0; for(const auto& ObjPac:ListaPacientes) Total+=ObjPac.Exp().Consultas().size();
        cout<<"Total consultas "<<Total<<"\n";
    }

    /* ---------- persistencia ---------- */
    void Guardar(){
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        ofstream FP("Pacientes.txt"), FM("Medicos.txt"), FE("Expedientes.txt");
        for(const auto& ObjPac:ListaPacientes) FP<<ObjPac.ATexto()<<"\n";
        for(const auto& ObjMed:ListaMedicos)   FM<<ObjMed.ATexto()<<"\n";
        for(const auto& ObjPac:ListaPacientes) FE<<"#"<<ObjPac.Id()<<"\n"<<ObjPac.Exp().ATexto()<<"$\n";
        cout<<"Guardado automatico\n";
    }
    void Cargar(){
        ifstream FP("Pacientes.txt"); string Linea;
        while(getline(FP,Linea) && !Linea.empty()) ListaPacientes.push_back(Paciente::DesdeTexto(Linea));
        ifstream FM("Medicos.txt");
        while(getline(FM,Linea) && !Linea.empty()) ListaMedicos.push_back(Medico::DesdeTexto(Linea));
        ifstream FE("Expedientes.txt");
        while(getline(FE,Linea)){
            if(!Linea.empty() && Linea[0]=='#'){
                int IdPac=stoi(Linea.substr(1)); Paciente* ObjPac=BuscarPaciente(IdPac);
                stringstream Buffer; string Aux;
                while(getline(FE,Aux) && Aux!="$") Buffer<<Aux<<"\n";
                if(ObjPac) ObjPac->Exp().DesdeStream(Buffer);
            }
        }
    }
};

/* ===================== hilos fondo ===================== */
void HiloGuardar(Hospital* Ptr){ while(ProgramaActivoFlag){ this_thread::sleep_for(chrono::minutes(5)); Ptr->Guardar(); } }
void HiloReporte(Hospital* Ptr){ while(ProgramaActivoFlag){ this_thread::sleep_for(chrono::minutes(7)); Ptr->Reporte(); } }

/* ===================== menus ===================== */
void MenuPrincipal(){ cout<<"\nSeleccione rol\n1 Paciente\n2 Doctor\n0 Salir\nOpcion: "; }
void MenuInicioPaciente(){ cout<<"\nPaciente\n1 Iniciar sesion\n2 Crear usuario\n0 Volver\nOpcion: "; }
void MenuInicioDoctor(){   cout<<"\nDoctor\n1 Iniciar sesion\n2 Crear usuario\n0 Volver\nOpcion: "; }
void MenuPaciente(){ cout<<"\n--- Menu Paciente ---\n1 Tomar turno\n0 Cerrar sesion\nOpcion: "; }
void MenuDoctor(){ cout<<"\n--- Menu Doctor ---\n1 Ver expediente\n2 Atender\n3 Reporte\n0 Cerrar sesion\nOpcion: "; }

/* ===================== main ===================== */
int main(){
    Hospital Sistema; Sistema.Cargar();
    thread TGuardar(HiloGuardar,&Sistema), TReporte(HiloReporte,&Sistema);

    bool Ejecutando=true;
    while(Ejecutando){
        MenuPrincipal(); int OpcionRol; cin>>OpcionRol; cin.ignore();
        if(OpcionRol==0){ Ejecutando=false; break; }

        /* ---------- flujo Paciente ---------- */
        if(OpcionRol==1){
            bool CicloInicio=true;
            while(CicloInicio){
                MenuInicioPaciente(); int OpIni; cin>>OpIni; cin.ignore();
                if(OpIni==0) break;
                if(OpIni==2){
                    Sistema.CrearPaciente();
                }else if(OpIni==1){
                    int IdPac=Hospital::LeerId("ID Paciente");
                    if(!Sistema.ExistePaciente(IdPac)){ cout<<"Id no valido\n"; continue; }
                    int OpPac=1;
                    while(OpPac!=0){
                        MenuPaciente(); cin>>OpPac; cin.ignore();
                        if(OpPac==1) Sistema.PacienteTomarTurno(IdPac);
                    }
                    CicloInicio=false;
                }
            }
        }

        /* ---------- flujo Doctor ---------- */
        if(OpcionRol==2){
            bool CicloInicio=true;
            while(CicloInicio){
                MenuInicioDoctor(); int OpIni; cin>>OpIni; cin.ignore();
                if(OpIni==0) break;
                if(OpIni==2){
                    Sistema.CrearMedico();
                }else if(OpIni==1){
                    int IdMed=Hospital::LeerId("ID Doctor");
                    if(!Sistema.ExisteMedico(IdMed)){ cout<<"Id no valido\n"; continue; }
                    int OpDoc=1;
                    while(OpDoc!=0){
                        MenuDoctor(); cin>>OpDoc; cin.ignore();
                        if(OpDoc==1) Sistema.VerExpediente();
                        else if(OpDoc==2) Sistema.Atender();
                        else if(OpDoc==3) Sistema.Reporte();
                    }
                    CicloInicio=false;
                }
            }
        }
    }

    ProgramaActivoFlag=false;
    TGuardar.join(); TReporte.join(); Sistema.Guardar();
    return 0;
}
