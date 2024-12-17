#[warn(dead_code)]
pub fn type_of<T>(_: &T) {
    format!("{}", std::any::type_name::<T>());
}

pub fn milliseconds_to_string(ms: u32) -> String {
    let millis = ms % 1000;
    let all_seconds = ms / 1000;
    let seconds = all_seconds % 60;
    let minutes = all_seconds / 60;
    let s = format!("{:3}:{:02}.{:03}", minutes, seconds, millis);
    s
}
