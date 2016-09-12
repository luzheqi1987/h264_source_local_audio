#include "moc_jerky_main_window.h"

int main(int argc, char* argv[]){
	QApplication  app(argc, argv);

	JerkyMainWindow main;

	main.show();

	return app.exec();
}