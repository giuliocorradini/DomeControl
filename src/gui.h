void GuiInit(void);
void GuiMain(void);
void ScreenUpdate(void);

void TelescopeDrawUpdate(int posizione);

void GuiHostLinkShow(int Ok);
void GuiEncLinkShow(int Ok);

void dummy(char * buff);

void GuiPage0BtnRestore(void);      //toglie eventuali pulsanti FERMA di pagina 0
void GuiPage0Btn0Stop(void);        //trasforma il pulsante 0 di page0 in FERMA
void GuiPage0Btn1Stop(void);        //trasforma il pulsante 1 di page0 in FERMA
