#![cfg(target_os = "android")]

use android_logger::Config;
use eframe::egui;
use log::LevelFilter;
use winit::platform::android::activity::AndroidApp;

enum ActionType{L1, L2, L3, L4, PROCESSOR, NET, ALGAE}
struct Action {
    typ: ActionType,
    auto: bool
}

struct App {
    actions: Vec<Action>,
    conditions: Vec<char>,
    pen_points: i8,
    auto_mode: bool,
    output: String
}
impl Default for App {
    fn default() -> Self {
	Self {actions: Vec::new(), conditions: Vec::new(), pen_points: 0, auto_mode: false, output: String::new()}
    }
}


fn android_main(app: AndroidApp) {
    android_logger::init_once(Config::default().with_max_level(LevelFilter::Info));

    let options = eframe::NativeOptions {
	android_app: Some(app),
	..Default::default()
    };
   
    eframe::run_native(
	"LER Scouting App".
	    options,
	Box::new(|cc| {
	    Ok(Box::<App>::default())
	}),
    ).unwrap()
	
}

impl eframe::App for App {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
	egui::CentralPanel::default().show(ctx, |ui| {
	    ui.heading("LER Scouting App");
	    ui.label(format!("label"));
	});
	
    }
}


