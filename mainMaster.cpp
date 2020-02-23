#include "pvm3.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include "imagen.h"
#include <iostream>
#include <iomanip>
#include "paquete.h"


using namespace std;

const int N = 100;
const char NOM_MAT[] = "matriz.txt";
const int MAX_SIZE = 24000000;
const int NPROCS = 5;


typedef double* Matriz;

void cargaLLave(Matriz& x, const string& s){
	ifstream f(s); 
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			f>>x[i*N+j];
			// cout<<x[i*N+j]<<" ";
		}
		//    cout<<endl;
	}
	f.close();
}

void crearMatrizTexto(Matriz& m, const char* mensaje, int size, int& R, int& C){
	R = N;
	C = size / N;
	int relleno = R * C - size;

	/* if(relleno){
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

unsigned char extraer(color* v){
	unsigned char c=0;
	c+=(v[0].r&1)*128;
	c+=(v[0].g&1)*64;
	c+=(v[0].b&1)*32;
	c+=(v[1].r&1)*16;
	c+=(v[1].g&1)*8;
	c+=(v[1].b&1)*4;
	c+=(v[2].r&1)*2;
	c+=(v[2].g&1)*1;
	return c;
}


int main(){
	int mytid, task_ids[NPROCS];
	int results[NPROCS], sum = 0;
	int i, msgtype;
	char SLAVE[] = "slave", EMPTY_STR[]=""; 
	/* enroll in PVM */
	mytid = pvm_mytid();

	/* spawn worker tasks */
	pvm_spawn(SLAVE, NULL, PvmTaskDefault, EMPTY_STR, NPROCS, task_ids);

	char *mensaje;
	ifstream arch("mensaje.txt");
	arch.seekg(0,arch.end);
	int size = arch.tellg();
	mensaje = new char[size];
	arch.seekg(0,arch.beg);
	arch.read(mensaje,size);
	// Ponerlo en la matriz
	// poner en el paquete
	// abrir imagen portadora
	imagen I;
	I.leerBMP("portador.bmp");
	PaqueteEntrada paq(I.columnas(), I.filas(), size, 0);
	paq.guardaImagen((unsigned char*)I.pixels);
	paq.guardaMensaje((unsigned char*)mensaje);
	paq.guardaTotal(NPROCS);


	/* send data to worker tasks */
	for (int dest = 0; dest < NPROCS; dest++) {
		pvm_initsend(PvmDataDefault);
		paq.guardaPosicion(dest);
		pvm_pkbyte((char*)paq.buffer,paq.buff_size,1);
		pvm_send(task_ids[dest], 4);
		printf("Se envio paquete a %d\n",dest);
	}

	/* wait and gather results */
	msgtype = 7;
	unsigned char *buffer = new unsigned char[I.filas()*I.columnas()];
	//memset(I.pixels,0,sizeof(color)*I.x*I.y);
	bool space = 0;
	int salto = 1;
	for (i = 0; i < NPROCS; i++) {
		pvm_recv(task_ids[i], msgtype);
		pvm_upkbyte((char*)buffer, I.x*I.y, 1);
		cout<<"Recibida trama"<<endl;
		PaqueteSalida ps;
		ps.buffer = buffer;
		int posBuffer = ps.numeroTrama();
		int tam = ps.tamMensaje();
		cout<<"Recibidos pixels de "<<posBuffer<<" a "<<posBuffer+tam<<endl;
		unsigned char*tmp = ps.punteroMensaje();
		memcpy(((unsigned char*)I.pixels)+posBuffer, ps.punteroMensaje(), tam);
	}
	incrustar(I.pixels, size&255);
	incrustar(I.pixels+3,size>>8);

	int R = N;
	int C = size / N;
	int relleno = R * C - size;

	/*if(relleno){
	C++;
	}*/

	unsigned char* ptr = (unsigned char*)&R;
	for(int i=0;i<sizeof(int);i++){
		incrustar(I.pixels+6+3*i, ptr[i]);
	}

	ptr = (unsigned char*)&C;
	for(int i=0;i<sizeof(int);i++){
		incrustar(I.pixels+6+3*sizeof(int)+3*i, ptr[i]);
	}


	I.guardaBMP("salida.bmp");


	pvm_exit();
	return 0;
}