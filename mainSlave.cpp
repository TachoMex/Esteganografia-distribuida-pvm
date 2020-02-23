#include "pvm3.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "paquete.h"
#include "imagen.h"
using namespace std;

const int N = 100;
const char NOM_MAT[] = "matriz.txt";
const int MAX_SIZE = 24000000;


typedef double* Matriz;

void cargaLLave(Matriz& x, const string& s){
	ifstream f(s); 
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			f>>x[i*N+j];
		//	cout<<x[i*N+j]<<" ";
		}
//		cout<<endl;
	}
	f.close();
}

void incrustar(color* v, unsigned char c){
	v[0].r=bool(c&128)+(v[0].r&254);
	v[0].g=bool(c&64)+ (v[0].g&254);
	v[0].b=bool(c&32)+ (v[0].b&254);
	v[1].r=bool(c&16)+ (v[1].r&254);
	v[1].g=bool(c&8)+  (v[1].g&254);
	v[1].b=bool(c&4)+  (v[1].b&254);
	v[2].r=bool(c&2)+  (v[2].r&254);
	v[2].g=bool(c&1)+  (v[2].g&254);
} 

void crearMatrizTexto(Matriz& m, const char* mensaje, int size, int& R, int& C){
	R = N;
	C = size / N;
	int relleno = R * C - size;

	/*	if(relleno){
		C++;
	}*/

	m = new double[R*C];
	memset(m,0,sizeof m);

	int k = 0;


	for(int i=0;i<R;i++){
		for(int j=0;j<C;j++){
			if(k>=size)
				m[i*C+j]=0.0;
			else
				m[i*C+j] = mensaje[k++];
		}
	}

}

int main(){
	int mytid;
	int i, sum, *a;
	int num_data, master;

	/* enroll in PVM */
	mytid = pvm_mytid();

	/* receive portion of array to be summed */
	pvm_recv(-1, -1);
	char *bufferEntrada = new char[MAX_SIZE];
	pvm_upkbyte((char*)bufferEntrada, MAX_SIZE, 1);

	PaqueteEntrada paq;
	paq.buffer = (unsigned char*)bufferEntrada;
	int tamMensaje = paq.tamMensaje();
	imagen I;
	I.pixels = (color*)paq.punteroImagen();
	I.y = paq.tamImagenX();
	I.x = paq.tamImagenY();

	int numTrama = paq.numeroTrama();
	printf("Se recibio el paquete en %d\n",numTrama);
	// Get how big the message is and put it in 'count'

	Matriz llave = new double[N*N];
	Matriz mensaje;
	int R, C;
	cargaLLave(llave, NOM_MAT);
	crearMatrizTexto(mensaje,(char*)paq.punteroMensaje(), tamMensaje, R, C);
	Matriz cifrado = new double[R*C];

	int totalTramas = paq.totalTramas();

	int Ri = R/totalTramas*(numTrama-1);
	int Rf = Ri+R/totalTramas;
	if(numTrama == totalTramas){
		Rf = R;
	}

	memset(cifrado,0,sizeof(cifrado));

	for(int i=Ri;i<Rf;i++){
		for(int j=0;j<C;j++){
			for(int k=0;k<N;k++){
				//cout<<llave[i*N+k]<<":"<<mensaje[k*C+j]<<" ";
				cifrado[i*C+j]+=llave[i*N+k]*mensaje[k*C+j];
			}
			cout<<cifrado[i*C+j]<<" ";
		}
		cout<<endl;
	}

	unsigned char *ptrMsjI = (unsigned char*)(cifrado+Ri*C);
	unsigned char *ptrMsjF = (unsigned char*)(cifrado+Rf*C);

	color* memoria = I.pixels + (2+sizeof(double)*Ri*C+sizeof(int)*2)*3;
	color* ptrPaquete = memoria;
	for(unsigned char * i = ptrMsjI; i < ptrMsjF; i++){
		incrustar(memoria, *i);
		memoria+=3;
	}
	
	cout<<"Bytes modificados: "<<memoria-ptrPaquete<<endl;

	PaqueteSalida ps((memoria-ptrPaquete)*9, (ptrPaquete-I.pixels)*3);
	ps.guardaMensaje((unsigned char*)ptrPaquete);
	bool space = false;
	/*int salto  =1;
	for(int i = 0;i<512;i++){
		cout<<hex<<setw(2)<<setfill('0')<<(int)(((unsigned char*)ptrMsjI)[i])<<(space?" ":"")<<(salto?"":"\n");
		space=!space;
		salto=(salto+1)%32;
	}*/

	cout<<"El proceso "<<numTrama<<" ha terminado y enviado resultado"<<endl;


	/* send computed sum back to master */
	master = pvm_parent();
	pvm_initsend(PvmDataRaw);
	pvm_pkbyte((char*)ps.buffer, ps.buff_size, 1);
	pvm_send(master, 7);
	I.pixels = NULL;
	pvm_exit();
}