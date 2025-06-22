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

using namespace std;


static int  SiguienteIdPaciente = 1;
static int  SiguienteIdMedico   = 1;
static bool ProgramaActivoFlag  = true;
mutex AccesoDatosMutex;


class Consulta{
public:
    string Fecha, Motivo, Diagnostico, Tratamiento, Notas;
    int IdMedico;
    Consulta() = default;
    Consulta(const string& FechaConsulta,const string& MotivoConsulta,const string& DiagnosticoConsulta,
             const string& TratamientoConsulta,const string& NotasConsulta,int IdMedicoConsulta) :
        Fecha(FechaConsulta),Motivo(MotivoConsulta),Diagnostico(DiagnosticoConsulta),
        Tratamiento(TratamientoConsulta),Notas(NotasConsulta),IdMedico(IdMedicoConsulta){}
    string ATexto() const{
        return Fecha+'|'+to_string(IdMedico)+'|'+Motivo+'|'+Diagnostico+'|'+Tratamiento+'|'+Notas;
    }
    static Consulta DesdeTexto(const string& Linea){
        stringstream Flujo(Linea);
        string FechaTxt,MotivoTxt,DiagTxt,TratTxt,NotaTxt,Aux;
        getline(Flujo,FechaTxt,'|'); getline(Flujo,Aux,'|'); int IdMedicoTxt=stoi(Aux);
        getline(Flujo,MotivoTxt,'|'); getline(Flujo,DiagTxt,'|');
        getline(Flujo,TratTxt,'|');  getline(Flujo,NotaTxt,'|');
        return Consulta(FechaTxt,MotivoTxt,DiagTxt,TratTxt,NotaTxt,IdMedicoTxt);
    }
};


class ExpedienteClinico{
    vector<Consulta> ListaConsultas;
public:
    void Agregar(const Consulta& Nueva){ ListaConsultas.insert(ListaConsultas.begin(),Nueva); }
    const vector<Consulta>& Consultas() const{ return ListaConsultas; }
    string ATexto() const{
        string Resultado; for(const auto& C:ListaConsultas) Resultado+=C.ATexto()+"\n"; return Resultado;
    }
    void DesdeStream(istream& Flujo){
        string Linea;
        while(getline(Flujo,Linea) && Linea!="$")
            if(!Linea.empty()) ListaConsultas.push_back(Consulta::DesdeTexto(Linea));
    }
};


class Paciente{
    int IdPaciente;
    string NombreCompleto, FechaNacimiento, Direccion, Identidad, Telefono, Correo, Genero;
    ExpedienteClinico Expediente;
public:
    Paciente() = default;
    Paciente(const string& Nombre,const string& Fecha,const string& DireccionTxt,const string& Ident,
             const string& TelefonoTxt,const string& CorreoTxt,const string& GeneroTxt) :
        IdPaciente(SiguienteIdPaciente++),NombreCompleto(Nombre),FechaNacimiento(Fecha),
        Direccion(DireccionTxt),Identidad(Ident),Telefono(TelefonoTxt),Correo(CorreoTxt),Genero(GeneroTxt){}
    int Id() const{ return IdPaciente; }
    const string& Nombre() const{ return NombreCompleto; }
    ExpedienteClinico& Exp(){ return Expediente; }
    const ExpedienteClinico& Exp() const{ return Expediente; }
    string ATexto() const{
        return to_string(IdPaciente)+'|'+NombreCompleto+'|'+FechaNacimiento+'|'+Direccion+'|'+Identidad+
               '|'+Telefono+'|'+Correo+'|'+Genero;
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


class Medico{
    int IdMedico;
    string NombreCompleto, Especialidad, NumeroColegiacion;
    bool DisponibleFlag;
public:
    Medico() = default;
    Medico(const string& Nombre,const string& EspTxt,const string& Numero) :
        IdMedico(SiguienteIdMedico++),NombreCompleto(Nombre),
        Especialidad(EspTxt),NumeroColegiacion(Numero),DisponibleFlag(true){}
    int Id() const{ return IdMedico; }
    const string& Nombre() const{ return NombreCompleto; }
    const string& Esp() const{ return Especialidad; }
    bool Disponible() const{ return DisponibleFlag; }
    void CambiarEstado(bool Valor){ DisponibleFlag=Valor; }
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


struct ColaEspecialidad{
    string NombreEspecialidad;
    vector<int> IdsPacienteVector;
};


class Hospital{
    vector<Paciente> ListaPacientes;
    vector<Medico>   ListaMedicos;
    vector<ColaEspecialidad> ListaColas;
    string AdminPassword;


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
    ColaEspecialidad* BuscarCola(const string& Esp){
        for(auto& ColaActual:ListaColas) if(ColaActual.NombreEspecialidad==Esp) return &ColaActual; return nullptr;
    }
    string NombreMedicoPorId(int IdMed){
        Medico* MedicoTemp=BuscarMedico(IdMed); return MedicoTemp?MedicoTemp->Nombre():string("SinRegistro");
    }

public:
   
    static int LeerId(const string& Etiqueta){
        cout<<Etiqueta<<": "; int Valor; cin>>Valor; cin.ignore(); return Valor;
    }
    bool ExistePaciente(int IdPac){
        lock_guard<mutex> Bloqueo(AccesoDatosMutex); return BuscarPaciente(IdPac)!=nullptr;
    }
    bool ExisteMedico(int IdMed){
        lock_guard<mutex> Bloqueo(AccesoDatosMutex); return BuscarMedico(IdMed)!=nullptr;
    }
    bool TieneAdmin() const{ return !AdminPassword.empty(); }

  
    int CrearPaciente(){
        string N,F,D,I,T,C,G;
        Campo("Nombre continuo",N); Campo("Fecha nacimiento",F); Campo("Direccion",D);
        Campo("Identidad",I); Campo("Telefono",T); Campo("Correo",C); Campo("Genero",G);
        lock_guard<mutex> L(AccesoDatosMutex);
        ListaPacientes.emplace_back(N,F,D,I,T,C,G);
        int IdNuevo=ListaPacientes.back().Id();
        cout<<"Paciente creado con ID "<<IdNuevo<<"\n";
        return IdNuevo;
    }
    int CrearMedico(){
        string N,E,Num; Campo("Nombre completo",N); Campo("Especialidad",E); Campo("Numero colegiacion",Num);
        lock_guard<mutex> L(AccesoDatosMutex);
        ListaMedicos.emplace_back(N,E,Num);
        int IdNuevo=ListaMedicos.back().Id();
        cout<<"Medico creado con ID "<<IdNuevo<<"\n";
        return IdNuevo;
    }
    void CrearAdmin(){
        if(TieneAdmin()){ cout<<"Ya existe un admin\n"; return; }
        string Pass1,Pass2;
        Campo("Contraseña admin",Pass1);
        Campo("Confirmar contraseña",Pass2);
        if(Pass1!=Pass2){ cout<<"No coincide\n"; return; }
        AdminPassword=Pass1; cout<<"Admin creado\n";
    }
    bool ValidarAdmin(){
        string Pass; Campo("Contraseña",Pass);
        if(Pass==AdminPassword) return true;
        cout<<"Contraseña incorrecta\n"; return false;
    }

    /* ---------- paciente opera ---------- */
    void PacienteTomarTurno(int IdPacienteSesion)
{
    {
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        for (const ColaEspecialidad& ColaActual : ListaColas)
        {
            auto Posicion = find(ColaActual.IdsPacienteVector.begin(),
                                 ColaActual.IdsPacienteVector.end(),
                                 IdPacienteSesion);
            if (Posicion != ColaActual.IdsPacienteVector.end())
            {
                cout << "Ya tiene un turno pendiente, Debe esperar a ser atendido\n";
                return;                                   
            }
        }
    }

    

    vector<Medico*> ListaDisponibles;
    {
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        for (auto& MedicoActual : ListaMedicos)
            if (MedicoActual.Disponible())
                ListaDisponibles.push_back(&MedicoActual);
    }
    if (ListaDisponibles.empty())
    {
        cout << "No hay doctores disponibles\n";
        return;
    }

    cout << "\nDoctores disponibles\n";
    for (size_t Indice = 0; Indice < ListaDisponibles.size(); ++Indice)
        cout << Indice + 1 << " "
             << ListaDisponibles[Indice]->Nombre() << "  "
             << ListaDisponibles[Indice]->Esp() << '\n';
    cout << "0 Regresar\nOpcion: ";

    int OpcionSeleccion;
    if (!(cin >> OpcionSeleccion))
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Entrada invalida\n";
        return;
    }
    cin.ignore();

    if (OpcionSeleccion == 0)            return;
    if (OpcionSeleccion < 1 ||
        OpcionSeleccion > static_cast<int>(ListaDisponibles.size()))
    {
        cout << "Opcion invalida\n";
        return;
    }

    Medico* DoctorElegido = ListaDisponibles[OpcionSeleccion - 1];
    {
        lock_guard<mutex> Bloqueo(AccesoDatosMutex);
        string EspecialidadDoctor = DoctorElegido->Esp();
        ColaEspecialidad* Cola = BuscarCola(EspecialidadDoctor);
        if (!Cola)
        {
            ListaColas.push_back({EspecialidadDoctor, {}});
            Cola = &ListaColas.back();
        }
        Cola->IdsPacienteVector.push_back(IdPacienteSesion);
    }
    cout << "Turno tomado con " << DoctorElegido->Nombre()
         << " especialidad "    << DoctorElegido->Esp() << '\n';
}

    
    void Atender(){
        int IdMedSesion=LeerId("Confirme su ID Doctor");
        lock_guard<mutex> L(AccesoDatosMutex);
        Medico* M=BuscarMedico(IdMedSesion);
        if(!M){ cout<<"Id incorrecto\n"; return; }
        string Esp=M->Esp(); ColaEspecialidad* Cola=BuscarCola(Esp);
        if(!Cola || Cola->IdsPacienteVector.empty()){ cout<<"Sin pacientes en cola\n"; return; }
        int IdPac=Cola->IdsPacienteVector.front(); Cola->IdsPacienteVector.erase(Cola->IdsPacienteVector.begin());
        Paciente* P=BuscarPaciente(IdPac);
        if(!P){ cout<<"Paciente no existe\n"; return; }
        M->CambiarEstado(false);

        string Mot,Dx,Tx,Nota; string Fh=FechaHoy();
        Campo("Motivo",Mot); Campo("Diagnostico",Dx); Campo("Tratamiento",Tx); Campo("Notas",Nota);
        P->Exp().Agregar(Consulta(Fh,Mot,Dx,Tx,Nota,M->Id()));
        M->CambiarEstado(true); cout<<"Consulta registrada\n";
    }
    void VerExpediente(){
        int IdPac=LeerId("ID Paciente"); lock_guard<mutex> L(AccesoDatosMutex);
        Paciente* P=BuscarPaciente(IdPac); if(!P){ cout<<"No existe\n"; return; }
        const auto& Vec=P->Exp().Consultas();
        if(Vec.empty()){ cout<<"Sin registros\n"; return; }
        cout<<"Expediente de "<<P->Nombre()<<" (ID "<<P->Id()<<")\n";
        for(const auto& C:Vec){
            cout<<"Fecha "<<C.Fecha<<"  Medico "<<NombreMedicoPorId(C.IdMedico)
                <<"  Motivo "<<C.Motivo<<"  Diagnostico "<<C.Diagnostico<<"\n";
        }
    }

    /* ---------- ADMIN opera ---------- */
    void AdminEliminarMedico(){
        int IdMed=LeerId("ID Doctor a eliminar");
        lock_guard<mutex> L(AccesoDatosMutex);
        auto It=remove_if(ListaMedicos.begin(),ListaMedicos.end(),
                          [&](const Medico& M){ return M.Id()==IdMed; });
        if(It!=ListaMedicos.end()){ ListaMedicos.erase(It,ListaMedicos.end()); cout<<"Doctor eliminado\n"; }
        else cout<<"No encontrado\n";
    }
    void AdminListarDoctores(){
        lock_guard<mutex> L(AccesoDatosMutex);
        cout<<"\nDoctores registrados\n";
        for(const auto& M:ListaMedicos)
            cout<<"Id "<<M.Id()<<"  "<<M.Nombre()<<"  "<<M.Esp()<<"  "<<(M.Disponible()?"Disponible":"Ocupado")<<"\n";
    }
   void AdminVerColaDoctor()
{
    int IdDoctor = LeerId("ID Doctor");
    lock_guard<mutex> Bloqueo(AccesoDatosMutex);

    Medico* DoctorSeleccionado = BuscarMedico(IdDoctor);
    if (!DoctorSeleccionado)
    {
        cout << "Doctor no encontrado\n";
        return;
    }

    ColaEspecialidad* ColaDelDoctor = BuscarCola(DoctorSeleccionado->Esp());
    if (!ColaDelDoctor || ColaDelDoctor->IdsPacienteVector.empty())
    {
        cout << "Cola vacia\n";
        return;
    }

    cout << "Cola de " << DoctorSeleccionado->Nombre()
         << "  " << DoctorSeleccionado->Esp() << '\n';

  
    constexpr int AnchoPosicion = 10;

    cout << left
         << setw(AnchoPosicion) << "Posicion"
         << "NombrePaciente\n";

    for (size_t Indice = 0; Indice < ColaDelDoctor->IdsPacienteVector.size(); ++Indice)
    {
        int IdPacienteActual = ColaDelDoctor->IdsPacienteVector[Indice];
        Paciente* PacienteActual = BuscarPaciente(IdPacienteActual);
        string NombrePaciente = PacienteActual ?
                                PacienteActual->Nombre() : "Desconocido";

        cout << left
             << setw(AnchoPosicion) << Indice + 1
             << NombrePaciente << '\n';
    }
}
    void AdminEliminarTurno(){
        int IdMed=LeerId("ID Doctor");
        lock_guard<mutex> L(AccesoDatosMutex);
        Medico* M=BuscarMedico(IdMed); if(!M){ cout<<"No existe\n"; return; }
        ColaEspecialidad* Cola=BuscarCola(M->Esp());
        if(!Cola || Cola->IdsPacienteVector.empty()){ cout<<"Cola vacia\n"; return; }
        cout<<"Posiciones en cola:\n";
        for(size_t i=0;i<Cola->IdsPacienteVector.size();++i){
            Paciente* P=BuscarPaciente(Cola->IdsPacienteVector[i]);
            cout<<i+1<<" "<<(P?P->Nombre():"Desconocido")<<"\n";
        }
        int Pos=LeerId("Numero a eliminar");
        if(Pos<1 || Pos>static_cast<int>(Cola->IdsPacienteVector.size())){ cout<<"Posicion invalida\n"; return; }
        Cola->IdsPacienteVector.erase(Cola->IdsPacienteVector.begin()+Pos-1);
        cout<<"Turno eliminado\n";
    }

    /* ---------- estadisticas ---------- */
    void Reporte(){
        lock_guard<mutex> L(AccesoDatosMutex);
        size_t Total=0; for(const auto& P:ListaPacientes) Total+=P.Exp().Consultas().size();
        cout<<"Total consultas "<<Total<<"\n";
    }

    /* -persistencia  */
    void Guardar(){
        lock_guard<mutex> L(AccesoDatosMutex);
        ofstream FP("Pacientes.txt"), FM("Medicos.txt"), FE("Expedientes.txt"), FA("Admin.txt");
        for(const auto& P:ListaPacientes) FP<<P.ATexto()<<"\n";
        for(const auto& M:ListaMedicos)   FM<<M.ATexto()<<"\n";
        for(const auto& P:ListaPacientes) FE<<"#"<<P.Id()<<"\n"<<P.Exp().ATexto()<<"$\n";
        FA<<AdminPassword<<"\n";
        cout<<"Guardado automatico\n";
    }
    void Cargar(){
        ifstream FP("Pacientes.txt"); string L;
        while(getline(FP,L) && !L.empty()) ListaPacientes.push_back(Paciente::DesdeTexto(L));
        ifstream FM("Medicos.txt");
        while(getline(FM,L) && !L.empty()) ListaMedicos.push_back(Medico::DesdeTexto(L));
        ifstream FE("Expedientes.txt");
        while(getline(FE,L)){ if(!L.empty() && L[0]=='#'){
            int IdPac=stoi(L.substr(1)); Paciente* P=BuscarPaciente(IdPac);
            stringstream Buf; string Aux;
            while(getline(FE,Aux) && Aux!="$") Buf<<Aux<<"\n";
            if(P) P->Exp().DesdeStream(Buf); } }
        ifstream FA("Admin.txt");
        getline(FA,AdminPassword);
    }
};

void HiloGuardar(Hospital* Ptr){ 
    while(ProgramaActivoFlag){ this_thread::sleep_for(chrono::minutes(5)); Ptr->Guardar(); } }
void HiloReporte(Hospital* Ptr){ while(ProgramaActivoFlag){ this_thread::sleep_for(chrono::minutes(7)); Ptr->Reporte(); 
} 
}

void MenuPrincipal()
{
    cout << "\nSeleccione rol\n"
         << "1 Paciente\n"
         << "2 Doctor\n"
         << "3 Admin\n"
         << "0 Salir\n"
         << "Opcion: ";
}

void MenuInicioPaciente()
{
    cout << "\n=== Paciente ===\n"
         << "1 Iniciar sesion\n"
         << "2 Crear usuario\n"
         << "0 Volver\n"
         << "Opcion: ";
}

void MenuInicioDoctor()
{
    cout << "\n=== Doctor ===\n"
         << "1 Iniciar sesion\n"
         << "2 Crear usuario\n"
         << "0 Volver\n"
         << "Opcion: ";
}

void MenuInicioAdmin(bool Existe)
{
    cout << "\n=== Admin ===\n"
         << "1 Iniciar sesion\n"
         << "2 Crear admin\n"
         << "0 Volver\n"
         << "Opcion: ";
    if (Existe)
        cout << "(Crear deshabilitado: ya existe un admin)\n";
}

void MenuPaciente()
{
    cout << "\n--- Menu Paciente ---\n"
         << "1 Tomar turno\n"
         << "0 Cerrar sesion\n"
         << "Opcion: ";
}

void MenuDoctor()
{
    cout << "\n--- Menu Doctor ---\n"
         << "1 Ver expediente\n"
         << "2 Atender\n"
         << "3 Reporte\n"
         << "0 Cerrar sesion\n"
         << "Opcion: ";
}

void MenuAdmin()
{
    cout << "\n--- Menu Admin ---\n"
         << "1 Ver doctores\n"
         << "2 Eliminar doctor\n"
         << "3 Ver cola doctor\n"
         << "4 Eliminar turno cola\n"
         << "5 Ver expediente paciente\n"
         << "0 Cerrar sesion\n"
         << "Opcion: ";
}

/* ================= main ============================== */
int main(){
    Hospital Sis; Sis.Cargar();
    thread TG(HiloGuardar,&Sis), TR(HiloReporte,&Sis);

    bool Ejecutando=true;
    while(Ejecutando){
        MenuPrincipal(); int OpRol; cin>>OpRol; cin.ignore();
        if(OpRol==0){ Ejecutando=false; break; }

        /* ===== flujo Paciente ===== */
        if(OpRol==1){
            bool Ciclo=true;
            while(Ciclo){
                MenuInicioPaciente(); int OpIni; cin>>OpIni; cin.ignore();
                if(OpIni==0) break;
                if(OpIni==2){ Sis.CrearPaciente(); }
                else if(OpIni==1){
                    int IdPac=Hospital::LeerId("ID Paciente");
                    if(!Sis.ExistePaciente(IdPac)){ cout<<"Id no valido\n"; continue; }
                    int OpPac=1;
                    while(OpPac!=0){ MenuPaciente(); cin>>OpPac; cin.ignore();
                        if(OpPac==1) Sis.PacienteTomarTurno(IdPac); }
                    Ciclo=false;
                }
            }
        }

        /* ===== flujo Doctor ===== */
        if(OpRol==2){
            bool Ciclo=true;
            while(Ciclo){
                MenuInicioDoctor(); int OpIni; cin>>OpIni; cin.ignore();
                if(OpIni==0) break;
                if(OpIni==2){ Sis.CrearMedico(); }
                else if(OpIni==1){
                    int IdMed=Hospital::LeerId("ID Doctor");
                    if(!Sis.ExisteMedico(IdMed)){ cout<<"Id no valido\n"; continue; }
                    int OpDoc=1;
                    while(OpDoc!=0){ MenuDoctor(); cin>>OpDoc; cin.ignore();
                        if(OpDoc==1) Sis.VerExpediente();
                        else if(OpDoc==2) Sis.Atender();
                        else if(OpDoc==3) Sis.Reporte(); }
                    Ciclo=false;
                }
            }
        }

            /* ===== flujo Admin ===== */
                if (OpRol == 3)
        {
            bool Ciclo = true;
            while (Ciclo)
            {
                MenuInicioAdmin(Sis.TieneAdmin());
                int OpIni; cin >> OpIni; cin.ignore();
                if (OpIni == 0) break;

                if (OpIni == 2)
                {
                    Sis.CrearAdmin();
                }
                else if (OpIni == 1)
                {
                    if (!Sis.TieneAdmin()) { cout << "No existe admin\n"; continue; }
                    if (!Sis.ValidarAdmin()) continue;

                    int OpAd = 1;
                    while (OpAd != 0)      
                    {
                        MenuAdmin();
                        cin >> OpAd;
                        cin.ignore();

                        if      (OpAd == 1) Sis.AdminListarDoctores();
                        else if (OpAd == 2) Sis.AdminEliminarMedico();
                        else if (OpAd == 3) Sis.AdminVerColaDoctor();
                        else if (OpAd == 4) Sis.AdminEliminarTurno();
                        else if (OpAd == 5) Sis.VerExpediente();       
                    }
                    Ciclo = false;   
                }
            }
        }
    }

    ProgramaActivoFlag=false;
    TG.join(); TR.join(); Sis.Guardar();
    return 0;
}