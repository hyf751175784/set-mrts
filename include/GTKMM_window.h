#ifndef GTKMM_WINDOW_H
#define GTKMM_WINDOW_H

#include <gtkmm.h>
#include <string>
//#include <gtkmm/window.h>
//#include <gtkmm/frame.h>

using namespace std;

class GTKMMWindow : public Gtk::Window
{
	private:
		string path;

		//typedef sigc::signal<void> Signals;
		
		//void reload();

	public:
		GTKMMWindow();
		virtual ~GTKMMWindow();
		//Signals refresh();
		
		void set_path(string path);

	protected:
		//Signals refresh_signal;
		//Child widgets:
		Gtk::Frame m_Frame;
		Gtk::Image m_Image;
};

#endif //GTKMM_WINDOW_H
