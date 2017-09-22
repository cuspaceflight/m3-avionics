export function msg_id(x){
    return x << 5;
}

export function bool(x){
    return !!x;
}

export function versionParser(subsystem){
    return (data) => {
        subsystem.version = String.fromCharCode.apply(null, data);
    };
}

