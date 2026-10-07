#include "fs_ia10b.h"
#include <stddef.h>
#include <string.h>
static FsIa10bStatus check(const FsIa10b *s) {
    if(s==NULL) return FS_IA10B_STATUS_INVALID_PARAM;
    return s->initialized ? FS_IA10B_STATUS_OK : FS_IA10B_STATUS_NOT_INITIALIZED;
}
static uint32_t lock(const FsIa10b *s){return s->config.ops->critical_enter(s->config.context);}
static void unlock(const FsIa10b *s,uint32_t k){s->config.ops->critical_exit(s->config.context,k);}
static bool arm(FsIa10b *s){return s->config.ops->start_receive(s->config.context,&s->rx_byte,1);}
/* Sliding candidate preserves embedded headers after corrupt or truncated frames */
static void process(FsIa10b *s,uint8_t byte,uint32_t time){
    s->frame[s->frame_index++]=byte;
    for(;;){
        if(s->frame_index==0) return;
        if(s->frame[0]!=0x20 || (s->frame_index>1&&s->frame[1]!=0x40)){
            memmove(s->frame,s->frame+1,--s->frame_index);continue;
        }
        if(s->frame_index<32) return;
        uint16_t sum=65535,channels[14];bool valid=true;
        for(unsigned i=0;i<30;i++) sum=(uint16_t)(sum-s->frame[i]);
        valid=sum==(uint16_t)(s->frame[30]|((uint16_t)s->frame[31]<<8));
        for(unsigned i=0;i<14;i++){
            channels[i]=(uint16_t)(s->frame[2+2*i]|((uint16_t)s->frame[3+2*i]<<8));
            if(channels[i]<800||channels[i]>2200) valid=false;
        }
        if(valid){memcpy(s->data.channel,channels,sizeof(channels));s->data.frame_count++;s->data.last_update_ms=time;s->data.valid=true;s->frame_index=0;return;}
        s->data.error_count++;memmove(s->frame,s->frame+1,--s->frame_index);
    }
}
FsIa10bStatus fs_ia10b_init(FsIa10b *s,const FsIa10bConfig *c){
    if(!s||!c||!c->ops||!c->ops->start_receive||!c->ops->abort_receive||!c->ops->now_ms||!c->ops->critical_enter||!c->ops->critical_exit) return FS_IA10B_STATUS_INVALID_PARAM;
    FsIa10bConfig cfg=*c;memset(s,0,sizeof(*s));s->config=cfg;
    if(!s->config.frame_timeout_ms)s->config.frame_timeout_ms=100;
    if(!s->config.restart_interval_ms)s->config.restart_interval_ms=200;
    s->initialized=true;s->last_restart_ms=c->ops->now_ms(c->context);
    s->restart_pending=!arm(s);
    return s->restart_pending?FS_IA10B_STATUS_PORT_ERROR:FS_IA10B_STATUS_OK;
}
FsIa10bStatus fs_ia10b_on_rx_complete(FsIa10b *s){
    FsIa10bStatus st=check(s);if(st)return st;uint32_t k=lock(s);
    if(!s->restart_pending){process(s,s->rx_byte,s->config.ops->now_ms(s->config.context));if(!arm(s)){s->restart_pending=true;s->data.error_count++;st=FS_IA10B_STATUS_PORT_ERROR;}}
    unlock(s,k);return st;
}
FsIa10bStatus fs_ia10b_on_rx_error(FsIa10b *s){
    FsIa10bStatus st=check(s);if(st)return st;uint32_t k=lock(s);
    s->data.error_count++;s->frame_index=0;s->restart_pending=true;unlock(s,k);return st;
}
FsIa10bStatus fs_ia10b_maintain(FsIa10b *s){
    FsIa10bStatus st=check(s);if(st)return st;uint32_t k=lock(s),time=s->config.ops->now_ms(s->config.context);
    uint32_t age=time-(s->data.valid?s->data.last_update_ms:s->last_restart_ms);
    bool stale=age>=s->config.frame_timeout_ms&&time-s->last_restart_ms>=s->config.restart_interval_ms;
    if(s->restart_pending||stale){
        s->restart_pending=true;s->frame_index=0;s->last_restart_ms=time;
        if(s->config.ops->abort_receive(s->config.context)&&arm(s))s->restart_pending=false;
        else st=FS_IA10B_STATUS_PORT_ERROR;
    }
    unlock(s,k);return st;
}
FsIa10bStatus fs_ia10b_get_data(const FsIa10b *s,FsIa10bData *out){
    if(!out)return FS_IA10B_STATUS_INVALID_PARAM;
    FsIa10bStatus st=check(s);if(st)return st;
    uint32_t k=lock(s);*out=s->data;unlock(s,k);return st;
}
bool fs_ia10b_is_online(const FsIa10b *s,uint32_t timeout){
    if(check(s))return false;
    uint32_t k=lock(s);
    bool online=s->data.valid&&!s->restart_pending&&(uint32_t)(s->config.ops->now_ms(s->config.context)-s->data.last_update_ms)<timeout;
    unlock(s,k);return online;
}
