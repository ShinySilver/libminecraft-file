#pragma once

namespace mcfile {

struct SetBlockOptions {
    SetBlockOptions()
        : fRemoveTileEntity(true)
    {}
    
    bool fRemoveTileEntity : 1;
};

} // namespace mcfile
