/* LER 2025 scouting app output generator */


extern crate ratatui;
extern crate crossterm;
use crossterm::event::{self, Event, KeyCode, KeyEvent, KeyEventKind};
use ratatui::{
    widgets::Paragraph,
    text::Line,
    DefaultTerminal, Frame,
};
use std::time::SystemTime;
use std::io;

#[derive(Clone, Copy)]
enum ActionType{L1, L2, L3, L4, PROCESSOR, NET, ALGAE}
struct Action {
    a: ActionType,
    t: i32,
    auto: bool
}

struct App {
    start_time: SystemTime,
    actions: Vec<Action>,
    conditions: Vec<char>,
    pen_points: i8,
    auto_mode: bool,
    quit: bool,
    output: String
}



impl App {
    fn run(&mut self, terminal: &mut DefaultTerminal) {
        while !self.quit {
            let _ = terminal.draw(|frame| self.draw(frame));
            match event::read().unwrap() {
                Event::Key(key_event) if key_event.kind == KeyEventKind::Press => {
                    self.handle_key(key_event)
                },
                _ => {}
            }
        }
    }
    fn handle_key(&mut self, key_event: KeyEvent) {
        match key_event.code {

            /* app */
            KeyCode::Char('q') => {self.quit = true},
            KeyCode::Char('g') => {self.output = self.generate_output(); self.quit = true},
            KeyCode::Char('t') => {self.auto_mode = !self.auto_mode},


            /* actions */
            KeyCode::Char('1') => {self.actions.push(Action{ a: ActionType::L1, t: self.start_time.elapsed().unwrap().as_millis() as i32, auto: self.auto_mode })},
            KeyCode::Char('2') => {self.actions.push(Action{ a: ActionType::L2, t: self.start_time.elapsed().unwrap().as_millis() as i32, auto: self.auto_mode })},
            KeyCode::Char('3') => {self.actions.push(Action{ a: ActionType::L3, t: self.start_time.elapsed().unwrap().as_millis() as i32, auto: self.auto_mode })},
            KeyCode::Char('4') => {self.actions.push(Action{ a: ActionType::L4, t: self.start_time.elapsed().unwrap().as_millis() as i32, auto: self.auto_mode })},
            KeyCode::Char('n') => {self.actions.push(Action{ a: ActionType::NET, t: self.start_time.elapsed().unwrap().as_millis() as i32, auto: self.auto_mode })},
            KeyCode::Char('p') => {self.actions.push(Action{ a: ActionType::PROCESSOR, t: self.start_time.elapsed().unwrap().as_millis() as i32, auto: self.auto_mode })},
            KeyCode::Char('m') => {self.actions.push(Action{ a: ActionType::ALGAE, t: self.start_time.elapsed().unwrap().as_millis() as i32, auto: self.auto_mode })},

            /* conditions */

            KeyCode::Char('l') => {if !self.conditions.contains(&'l') {self.conditions.push('l')}} // played match
            KeyCode::Char('w') => {if !self.conditions.contains(&'w') {self.conditions.push('w')}} // won game
            KeyCode::Char('e') => {if !self.conditions.contains(&'e') {self.conditions.push('e')}} // played defence
            KeyCode::Char('a') => {if !self.conditions.contains(&'a') {self.conditions.push('a')}} // had auto (could leave start area)
            KeyCode::Char('d') => {if !self.conditions.contains(&'d') {self.conditions.push('d')}} // did deep climb
            KeyCode::Char('s') => {if !self.conditions.contains(&'s') {self.conditions.push('s')}} // did shallow climb
            KeyCode::Char('r') => {if !self.conditions.contains(&'r') {self.conditions.push('r')}} // did park



            _ => {}
        }
    }

    fn draw(&self, frame: &mut Frame) {
        let size = frame.area();
        let mut s: Vec<Line> = Vec::new();
        s.push(Line::from("test"));
        if self.auto_mode == true {
            s.push(Line::from("auto: true"))
        } else {
            s.push(Line::from("auto: false"))
        }
        s.push(Line::from("\n\n\n action list: "));
        for i in &self.actions {
            s.push(Line::from(format!("{} @ {}, auto: {}", App::action_type_string(i.a.clone()), i.t, i.auto)))
        }
        s.push(Line::from("\n\n\n condition list: "));
        let c: String = self.conditions.iter().collect();
        s.push(Line::from(c));

        s.push(Line::from("\n\napp: [qg] act: [1234pnf] cond: [wadsr]"));
        frame.render_widget(Paragraph::new(s), size);
    }

    fn action_type_string(x: ActionType) -> String {
        match x {
            ActionType::L1 => {return "L1".to_string()},
            ActionType::L2 => {return "L2".to_string()},
            ActionType::L3 => {return "L3".to_string()},
            ActionType::L4 => {return "L4".to_string()},
            ActionType::PROCESSOR => {return "proc".to_string()},
            ActionType::ALGAE => {return "algae".to_string()},
            ActionType::NET => {return "net".to_string()},

        }
    }

    // fn generate_string(&mut self) {
    //     let mut s: String = String::new();

    //     s.push_str(format!("{} ", 1).as_str()); // match #
    //     s.push_str(format!("{}", 2708).as_str()); // team #

    //     let c: String = self.conditions.iter().collect();
    //     s.push_str(format!("{}>", c).as_str());

    //     for i in &self.actions {
    //         let mut ss: String = String::new();
    //         match i.a {
    //            ActionType::L1 => {if i.auto { ss.push('O') } else { ss.push('o') }},
    //            ActionType::L2 => {if i.auto { ss.push('T') } else { ss.push('t') }},
    //            ActionType::L3 => {if i.auto { ss.push('H') } else { ss.push('h') }},
    //            ActionType::L4 => {if i.auto { ss.push('F') } else { ss.push('o') }},
    //            ActionType::NET => {if i.auto { ss.push('N') } else { ss.push('n') }},
    //            ActionType::PROCESSOR => {if i.auto { ss.push('P') } else { ss.push('p') }},
    //             ActionType::FOUL => {ss.push('u')},
    //         }
    //         ss.push_str(i.t.to_string().as_str());
    //         // ss.push_str(">");
    //         s.push_str(ss.as_str());
    //     }
    //     s.push_str("|");
    //     self.output = s;
    //     self.quit = true;
    // }

    fn generate_output(&mut self) -> String {
        let mut s: String = String::new();

        /* match # */
        s.push_str(format!("{:08b}", 5).as_str());

        /* team # */
        s.push_str(format!("{:016b}", 2708).as_str());

	/* team penalty points */
	s.push_str(format!("{:08b}", self.pen_points).as_str());
	
	
        /* conditions */
        for i in &self.conditions {
            s.push_str(App::condition_type_bin(i));
        }
        s.push_str("000");

        /* actions */
        for i in &self.actions {
            s.push_str(App::action_type_bin(i));
        }
        println!("output generated");
        return s;
    }

    fn action_type_bin(x: &Action) -> &str {
        match x.a {
            ActionType::L1 => {if x.auto { return "0001" } else { return "0010" }},
            ActionType::L2 => {if x.auto { return "0100" } else { return "1000" }},
            ActionType::L3 => {if x.auto { return "0011" } else { return "1100" }},
            ActionType::L4 => {if x.auto { return "1001" } else { return "0110" }},
            ActionType::NET => {if x.auto { return "1010" } else { return "0101" }},
            ActionType::PROCESSOR => {if x.auto { return "0111" } else { return "1110" }},
            ActionType::ALGAE => {if x.auto { return "1011" } else { return "1101" }},
        }
    }

    fn condition_type_bin(x: &char) -> &str {
        match x {
            'l' => {return "001"}, //played match
            'w' => {return "010"}, // won match
            'e' => {return "100"}, // played defense
            'a' => {return "011"}, // had auto (left start zone)
            'd' => {return "110"}, // did deep climb
            's' => {return "101"}, // did shallow climb
            'r' => {return "010"}, // did park
            _ => {return "111"}
        }
    }
}



fn main() {
    let mut terminal: DefaultTerminal = ratatui::init();
    let mut app = App {
        start_time: SystemTime::now(),
        actions: Vec::new(),
        conditions: Vec::new(),
	pen_points: 0,
        auto_mode: true,
        quit: false,
        output: "NONE".to_string()
    };
    app.run(&mut terminal);
    ratatui::restore();
    println!("{}", app.output);
}
