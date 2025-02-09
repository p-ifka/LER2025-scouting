extern crate serde_json;

use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
struct BinConversion {
    cond_conversion: Vec<[String; 2]>
}
