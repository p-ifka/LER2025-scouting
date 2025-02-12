use std::env;

#[derive(Debug)]
enum Check{PLAYED, WON, DEFENSE, AUTO, DEEP, SHALLOW, PARK}


fn main() {
    let args: Vec<String> = env::args().collect(); 
    if args.len() < 2 {
	println!("no output provided");
    } else {
	let c: String = args[1].to_string();
	println!("parsing data: {}", c);
	// let c: Vec<char> = args[1].chars().collect();
	let mut point: i16 = 0;


	let match_num_bin = &c[point as usize..8];
	let match_num = i8::from_str_radix(match_num_bin, 2).expect("match_num not a valid binary number");
	point += 8;
	println!("match number: {} -> {}", match_num_bin, match_num);

	let team_num_bin = &c[point as usize..(point+16) as usize];
	let team_num = i16::from_str_radix(team_num_bin, 2).expect("team_num not a valid binary number");
	point += 16;
	println!("team number: {} -> {}", team_num_bin, team_num);


	let mut checks: Vec<Check> = Vec::new();
	let mut checks_continue = true;
	while checks_continue {
	    match &c[point as usize..(point+3) as usize] {
		"000" => {point += 3; println!("reached end of checks"); checks_continue = false; break; },
		"001" => {checks.push(Check::PLAYED)},
		"010" => {checks.push(Check::WON)},
		"100" => {checks.push(Check::DEFENSE)}
		"110" => {checks.push(Check::DEEP)}
		"011" => {checks.push(Check::SHALLOW)}
		"101" => {checks.push(Check::PARK)}
		_=> {println!("err: invalid condition")}
	    }
	    point += 3;
	}
	dbg!(checks);
	
	}
    }
    
