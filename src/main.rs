/* LER 2025 scouting app test
 *  binds:
 * app: q; quit, t; toggle auto mode, g; generate output
 * actions: 1234; coral levels, n; net, p; processor
 * conditions: w; win, a; had auto, s; shallow climb, d; deep climb, r; park
 *
 * press above buttons to add action/conditions to list, press g to generate output,
 * press t to toggle auto mode, if auto mode is on the action will be recorded as done in autonomous
 * if applicable
 *
 * */



extern crate ratatui;
extern crate crossterm;
use crossterm::event::{self, Event, KeyCode, KeyEvent, KeyEventKind};
use ratatui::{
    widgets::Paragraph,
    text::Line,
    DefaultTerminal, Frame,
};
use std::time::SystemTime;

#[derive(Clone, Copy)]
enum ActionType{L1, L2, L3, L4, PROCESSOR, NET, FOUL}
struct Action {
    a: ActionType,
    t: u128,
    points: i32,
    auto: bool
}

struct App {
    start_time: SystemTime,
    actions: Vec<Action>,
    conditions: Vec<char>,
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
            KeyCode::Char('g') => {self.generate_string()},
            KeyCode::Char('t') => {self.auto_mode = !self.auto_mode},


            /* actions */
            KeyCode::Char('1') => {self.actions.push(Action{ a: ActionType::L1, t: self.start_time.elapsed().unwrap().as_millis(), points: 1, auto: self.auto_mode })},
            KeyCode::Char('2') => {self.actions.push(Action{ a: ActionType::L2, t: self.start_time.elapsed().unwrap().as_millis(), points: 1, auto: self.auto_mode })},
            KeyCode::Char('3') => {self.actions.push(Action{ a: ActionType::L3, t: self.start_time.elapsed().unwrap().as_millis(), points: 1, auto: self.auto_mode })},
            KeyCode::Char('4') => {self.actions.push(Action{ a: ActionType::L4, t: self.start_time.elapsed().unwrap().as_millis(), points: 1, auto: self.auto_mode })},
            KeyCode::Char('n') => {self.actions.push(Action{ a: ActionType::NET, t: self.start_time.elapsed().unwrap().as_millis(), points: 1, auto: self.auto_mode })},
            KeyCode::Char('p') => {self.actions.push(Action{ a: ActionType::PROCESSOR, t: self.start_time.elapsed().unwrap().as_millis(), points: 1, auto: self.auto_mode })},
            KeyCode::Char('f') => {self.actions.push(Action{ a: ActionType::FOUL, t: self.start_time.elapsed().unwrap().as_millis(), points: 0, auto: false})},

            /* conditions */
            KeyCode::Char('w') => {if !self.conditions.contains(&'w') {self.conditions.push('w')}} // won game
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
            s.push(Line::from(format!("{}: {} @ {}, auto: {}", App::action_type_string(i.a.clone()), i.points, i.t, i.auto)))
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
            ActionType::NET => {return "net".to_string()},
            ActionType::FOUL => {return "foul".to_string()}

        }
    }

    fn generate_string(&mut self) {
        let mut s: String = String::new();

        s.push_str(format!("{} ", 1).as_str()); // match #
        s.push_str(format!("{}", 2708).as_str()); // team #

        let c: String = self.conditions.iter().collect();
        s.push_str(format!("{}>", c).as_str());

        for i in &self.actions {
            let mut ss: String = String::new();
            match i.a {
               ActionType::L1 => {if i.auto { ss.push('O') } else { ss.push('o') }},
               ActionType::L2 => {if i.auto { ss.push('T') } else { ss.push('t') }},
               ActionType::L3 => {if i.auto { ss.push('H') } else { ss.push('h') }},
               ActionType::L4 => {if i.auto { ss.push('F') } else { ss.push('o') }},
               ActionType::NET => {if i.auto { ss.push('N') } else { ss.push('n') }},
               ActionType::PROCESSOR => {if i.auto { ss.push('P') } else { ss.push('p') }},
                ActionType::FOUL => {ss.push('u')},
            }
            ss.push_str(i.t.to_string().as_str());
            ss.push_str(">");
            s.push_str(ss.as_str());
        }
        s.push_str("|");
        self.output = s;
        self.quit = true;
    }

}

fn main() {
    let mut terminal: DefaultTerminal = ratatui::init();
    let mut app = App {
        start_time: SystemTime::now(),
        actions: Vec::new(),
        conditions: Vec::new(),
        auto_mode: true,
        quit: false,
        output: "NONE".to_string()
    };
    app.run(&mut terminal);
    ratatui::restore();
    println!("{}", app.output);
}
