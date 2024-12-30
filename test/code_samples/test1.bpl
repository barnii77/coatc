let glob_var;

// this is a comment
extern fn main(b) {
    let a = 3;
    b = add1(a * 0x3f - 2);
    if (1 - a) {
        //return 1;
        0
    } else {
        -a
    }
    while (b) {
        print(b);
    }
    print(a);
    return 0x42;
}

externc fn add1(x) {
    return x + 1;
}
