/*! \mainpage Ejercicio Titulo
 * \date 10/09/2023
 * \author Alejandro Rougier
 * \section Ejemplo comunicación USART
 * [Complete aqui con su descripcion]
 *
 * \section desarrollos Observaciones generales
 * [Complete aqui con sus observaciones]
 *
 * \section changelog Registro de cambios
 *
 * |   Fecha    | Descripcion                                    |
 * |:----------:|:-----------------------------------------------|
 * | 10/09/2023 | Creacion del documento                         |
 *
 */



/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "util.h"
#include "myDelay.h"
#include "debounce.h"
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/

/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/
#define RXBUFSIZE  128
#define TXBUFSIZE  128

#define DEBOUNCE    20
#define HEARBEATIME 100
#define GENERALTIME 10
#define NUMBUTTONS  4
#define INTERVAL100MS   100
#define INTERVAL200MS   200
#define INTERVAL2000MS  2000
#define INTERVAL1000MS  1000
#define INTERVAL500MS   500

#define TMIN            100
#define TMAX            500

#define ISCOMAND    flags.bits.bit0
#define RESETFLAGS  flags.bytes 

/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/

DigitalOut LED(PC_13);

BusOut leds(PB_6,PB_7,PB_14,PB_15);

BusIn   pulsadores(PA_4,PA_5,PA_6,PA_7);

RawSerial PC(PA_9,PA_10);

/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/

/**
 * @brief Hearbeat, indica el funcionamiento del sistema
 * 
 * @param timeHearbeat Variable para el intervalo de tiempo
 * @param mask Secuencia de encendido/apagado del led de Hearbeat
 */
void hearbeatTask(_delay_t *timeHearbeat, uint16_t mask);

/**
 * @brief Ejecuta las tareas del puerto serie Decodificación/trasnmisión
 * 
 * @param dataRx Estructura de datos de la recepción
 * @param dataTx Estructura de datos de la trasnmisión
 */
void serialTask(_sRx *dataRx, _sTx *dataTx);

/**
 * @brief Recepción de datos por el puerto serie
 * 
 */
void onRxData();

/**
 * @brief Pone el encabezado del protocolo, el ID y la cantidad de bytes a enviar
 * 
 * @param dataTx Estructura para la trasnmisión de datos
 * @param ID Identificación del comando que se envía
 * @param frameLength Longitud de la trama del comando
 * @return uint8_t devuelve el Checksum de los datos agregados al buffer de trasnmisión
 */
uint8_t putHeaderOnTx(_sTx  *dataTx, _eCmd ID, uint8_t frameLength);

/**
 * @brief Agrega un byte al buffer de transmisión
 * 
 * @param dataTx Estructura para la trasnmisión de datos
 * @param byte El elemento que se quiere agregar
 * @return uint8_t devuelve el Checksum del dato agregado al buffer de trasnmisión
 */
uint8_t putByteOnTx(_sTx    *dataTx, uint8_t byte);

/**
 * @brief Agrega un String al buffer de transmisión
 * 
 * @param dataTx Estructura para la trasnmisión de datos
 * @param str String a agregar
 * @return uint8_t devuelve el Checksum del dato agregado al buffer de trasnmisión
 */
uint8_t putStrOntx(_sTx *dataTx, const char *str);

/**
 * @brief Decodifica la trama recibida
 * 
 * @param dataRx Estructura para la recepción de datos
 */
void decodeHeader(_sRx *dataRx);

/**
 * @brief Decodifica el comando recibido en la transmisión y ejecuita las tareas asociadas a dicho comando
 * 
 * @param dataRx Estructura para la recepción de datos
 * @param dataTx Estructura para la trasnmisión de datos
 */
void decodeCommand(_sRx *dataRx, _sTx *dataTx);


/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/

const uint8_t secuencia[7]={0x01,0x03,0x02,0x06,0x04,0x0C,0x08};

const char firmware[] = "EX100923.01";

volatile _sRx dataRx;

_sTx dataTx;

volatile uint8_t buffRx[RXBUFSIZE];

uint8_t buffTx[TXBUFSIZE];

uint8_t globalIndex, index2;

_uFlag  flags;

_sButton myButton[NUMBUTTONS];

_sleds  dataLeds;

//_sGame  dataGame;

_delay_t    timeGlobal;

_delay_t    generalTime;

Timer myTimer;

/* END Global variables ------------------------------------------------------*/


/* Function prototypes user code ----------------------------------------------*/
void hearbeatTask(_delay_t *timeHearbeat, uint16_t mask)
{
    static uint8_t sec=0;
    if(delayRead(timeHearbeat)){
        LED = (~mask & (1<<sec));
        sec++;
        sec &= -(sec<16);
    }
}

void serialTask(_sRx *dataRx, _sTx *dataTx)
{
    if(ISCOMAND){
        ISCOMAND=false;
        decodeCommand(dataRx,dataTx);
    }

    if(delayRead(&generalTime)){
        if(dataRx->header){
            dataRx->timeOut--;
        if(!dataRx->timeOut)
            dataRx->header = HEADER_U;
        }
    }

    if(dataRx->indexR!=dataRx->indexW){
        decodeHeader(dataRx);
        /*
        //CODIGO A EFECTOS DE EVALUAR SI FUNCIONA LA RECEPCIÓN , SE DEBE DESCOMENTAR Y COMENTAR LA LINEA decodeHeader(dataRx); 
       
       while (dataRx->indexR!=dataRx->indexW){
            dataTx->buff[dataTx->indexW++]=dataRx->buff[dataRx->indexR++];
            dataTx->indexW &= dataTx->mask;
            dataRx->indexR &= dataRx->mask;
        } 
        */
    }
        
    if(dataTx->indexR!=dataTx->indexW){
        if(PC.writeable()){
            PC.putc(dataTx->buff[dataTx->indexR++]);
            dataTx->indexR &=dataTx->mask;
        }
    }

}

void onRxData()
{
    while(PC.readable()){
        dataRx.buff[dataRx.indexW++]=PC.getc();
        dataRx.indexW &= dataRx.mask;
    }
}

uint8_t putHeaderOnTx(_sTx  *dataTx, _eCmd ID, uint8_t frameLength)
{
    dataTx->chk = 0;
    dataTx->buff[dataTx->indexW++]='U';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]='N';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]='E';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]='R';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]=frameLength+1;
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]=':';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]=ID;
    dataTx->indexW &= dataTx->mask;
    dataTx->chk ^= (frameLength+1);
    dataTx->chk ^= ('U' ^'N' ^'E' ^'R' ^ID ^':') ;
    return  dataTx->chk;
}

uint8_t putByteOnTx(_sTx *dataTx, uint8_t byte)
{
    dataTx->buff[dataTx->indexW++]=byte;
    dataTx->indexW &= dataTx->mask;
    dataTx->chk ^= byte;
    return dataTx->chk;
}

uint8_t putStrOntx(_sTx *dataTx, const char *str)
{
    globalIndex=0;
    while(str[globalIndex]){
        dataTx->buff[dataTx->indexW++]=str[globalIndex];
        dataTx->indexW &= dataTx->mask;
        dataTx->chk ^= str[globalIndex++];
    }
    return dataTx->chk ;
}

void decodeHeader(_sRx *dataRx)
{
    uint8_t auxIndex=dataRx->indexW;
    while(dataRx->indexR != auxIndex){
        switch(dataRx->header)
        {
            case HEADER_U:
                if(dataRx->buff[dataRx->indexR] == 'U'){
                    dataRx->header = HEADER_N;
                    dataRx->timeOut = 5;
                }
            break;
            case HEADER_N:
                if(dataRx->buff[dataRx->indexR] == 'N'){
                    dataRx->header = HEADER_E;
                }else{
                    if(dataRx->buff[dataRx->indexR] != 'U'){
                        dataRx->header = HEADER_U;
                        dataRx->indexR--;
                    }
                }
            break;
            case HEADER_E:
                if(dataRx->buff[dataRx->indexR] == 'E'){
                    dataRx->header = HEADER_R;
                }else{
                    dataRx->header = HEADER_U;
                    dataRx->indexR--;
                }
            break;
            case HEADER_R:
                if(dataRx->buff[dataRx->indexR] == 'R'){
                    dataRx->header = NBYTES;
                }else{
                    dataRx->header = HEADER_U;
                    dataRx->indexR--;
                }
            break;
            case NBYTES:
                dataRx->nBytes=dataRx->buff[dataRx->indexR];
                dataRx->header = TOKEN;
            break;
            case TOKEN:
                if(dataRx->buff[dataRx->indexR] == ':'){
                    dataRx->header = PAYLOAD;
                    dataRx->indexData = dataRx->indexR+1;
                    dataRx->indexData &= dataRx->mask;
                    dataRx->chk = 0;
                    dataRx->chk ^= ('U' ^'N' ^'E' ^'R' ^dataRx->nBytes ^':') ;
                }else{
                    dataRx->header = HEADER_U;
                    dataRx->indexR--;
                }
            break;
            case PAYLOAD:
                dataRx->nBytes--;
                if(dataRx->nBytes>0){
                   dataRx->chk ^= dataRx->buff[dataRx->indexR];
                }else{
                    dataRx->header = HEADER_U;
                    if(dataRx->buff[dataRx->indexR] == dataRx->chk)
                        ISCOMAND = true;
                }
            break;
            default:
                dataRx->header = HEADER_U;
            break;
        }
        dataRx->indexR++;
        dataRx->indexR &= dataRx->mask;
    }
}

void decodeCommand(_sRx *dataRx, _sTx *dataTx)
{
    switch(dataRx->buff[dataRx->indexData]){
        case ALIVE: //seria lo mismo que tener 0xF0
            putHeaderOnTx(dataTx, ALIVE, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case FIRMWARE: //mismo que tener 0xF1
            putStrOntx(dataTx, "+&DBG");
            putStrOntx(dataTx, firmware);
            putByteOnTx(dataTx, '\n');
        break;
        case LEDSTATUS:
        // NO ESTA CONTEMPLANDO EL ENCENDIDO/APAGADO DE LOS LEDS, SOLO EL ESTADO
            putHeaderOnTx(dataTx, LEDSTATUS, 2);
            putByteOnTx(dataTx, ((~((uint8_t)leds.read()))&0x0F));
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case BUTTONSTATUS:
            putHeaderOnTx(dataTx, BUTTONSTATUS, 2);
            putByteOnTx(dataTx, ((~((uint8_t)pulsadores.read()))&0x0F));
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case DEGREES:
        break;
        case VELOCITY:
        break;
        case POSITION:
            putHeaderOnTx(dataTx, POSITION, 2);
            putByteOnTx(dataTx, ((~((uint8_t)pulsadores.read()))&0x0F));
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case LAUNCHSTATE:
            putHeaderOnTx(dataTx, LAUNCHSTATE, 2);
            putByteOnTx(dataTx, ((~((uint8_t)pulsadores.read()))&0x0F));
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case PARAMCHANGE:
        break;
        case WALLBOUNCE:
            //dependiendo de los si choca la pares se prenden los leds
            if (dataRx->buff[dataRx->indexData+1] == 0x08) {   // FLOOR
                leds = 0x08;
            } else if (dataRx->buff[dataRx->indexData+1] == 0x04) { // ROOF
                leds = 0x04;
            } else if (dataRx->buff[dataRx->indexData+1] == 0x02) { // LEFT
                leds = 0x02;
            } else if (dataRx->buff[dataRx->indexData+1] == 0x01) { // RIGHT
                leds = 0x01;
            }
        break;
        default://comando desconocido
            putHeaderOnTx(dataTx, (_eCmd)dataRx->buff[dataRx->indexData], 2);
            putByteOnTx(dataTx,UNKNOWN );
            putByteOnTx(dataTx, dataTx->chk);
        break;
        
    }


}


/* END Function prototypes user code ------------------------------------------*/

int main()
{
    dataRx.buff = (uint8_t *)buffRx;
    dataRx.indexR = 0;
    dataRx.indexW = 0;
    dataRx.header = HEADER_U;
    dataRx.mask = RXBUFSIZE - 1;

    dataTx.buff = buffTx;
    dataTx.indexR = 0;
    dataTx.indexW = 0;
    dataTx.mask = TXBUFSIZE -1;
/*
    dataGame.stateGame=START;
    dataGame.level = 4;
    dataGame.playerState = NOTDEFINE;
*/
    RESETFLAGS = 0;

/* Local variables -----------------------------------------------------------*/
    uint16_t    mask = 0x000A;
    _delay_t    hearbeatTime;
    _delay_t    debounceTime;
   
/* END Local variables -------------------------------------------------------*/


/* User code -----------------------------------------------------------------*/
    PC.baud(115200);
    PC.attach(&onRxData, SerialBase::IrqType::RxIrq);
    myTimer.start();

    delayConfig(&hearbeatTime, HEARBEATIME);
    delayConfig(&debounceTime, DEBOUNCE);
    delayConfig(&generalTime,GENERALTIME);
    delayConfig(&timeGlobal, GENERALTIME );
    delayConfig(&dataLeds.time, INTERVAL200MS);
    startButon(myButton, NUMBUTTONS);


    while(1){
        hearbeatTask(&hearbeatTime, mask);
        serialTask((_sRx *)&dataRx,&dataTx);
        buttonTask(&debounceTime,myButton, pulsadores.read());

    }

/* END User code -------------------------------------------------------------*/
}


