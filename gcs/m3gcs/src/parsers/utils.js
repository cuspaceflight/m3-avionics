export function msg_id(x){
    return x << 5;
}

export function bool(x){
    return !!x;
}

export function versionParser(subsystem){
    return (data) => {
        if (subsystem.version instanceof DataPoint) {
            subsystem.version.set(String.fromCharCode.apply(null, data));
        }
    };
}

export class DataPoint {
    constructor(def, parent=null) {
        this.parent = parent;
        if (typeof def === 'object'){
            var nv = {};
            for (var k in def) {
                nv[k] = new DataPoint(def[k], this);
            }
            this.value = nv;
        }else{
            this.value = def;
        }
        this.lastUpdate = 0;
    }

    set(v, propagate=true) {
        if (typeof v === 'object'){
            for (var k in v) {
                if (this.value[k] === undefined){
                    this.value[k] = new DataPoint(v[k], this);
                }
                this.value[k].set(v[k], false);
            }
        }else{
            this.value = v;
        }
        this.lastUpdate = new Date();
        if(propagate && this.parent !== null){
            this.parent.setLastUpdate(this.lastUpdate);
        }
    }

    setLastUpdate(date){
        if(this.parent !== null){
            this.parent.setLastUpdate(date);
        }
    }

    get() {
        return this.value;
    }

    updateTime() {
        return this.lastUpdate;
    }
}
