void del3us(int tt);//задержка на пропуске тактов
int sensor_rst(void); //функция сброса шины onewire
void sensor_init(PORT*p, char pin);//функция инициализации датчика
int sensor_ReadBit(void); // считывание 1 бита
int sensor_ReadByte(void);  //считывание целого байта информации
int sensor_WriteBit(int bit); //передача бита
int sensor_WriteByte(int byte); //передача целого байта информации
float sensor_get_temp(char*a); //функция считывания температуры с датчика
char* sensor_get_add(void);//функция получения адреса с датчика
void temp_transmit(void);//функция запуска процедуры оцифровки значения
char read_curent__add(void); //функция передачи одного байта адреса
