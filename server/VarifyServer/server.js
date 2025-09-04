const grpc = require('@grpc/grpc-js')
const message_proto = require('./proto')
const const_module = require("./const")
const { v4: uuidv4  } = require("uuid")
const emailModule = require("./email")
const redis_module = require('./redis')

//实现了获取验证码服务函数
async function GetVarifyCode(call, callback) {
    console.log("email is ", call.request.email)
    //promise返回的结果有两种，reject和resolve两种，一种是异常一种是正常
    try{
        //需要先通过email查询redis（之前有没有发过验证码）存不存在，如果不存在再执行创建uuid
        let query_res = await redis_module.GetRedis(const_module.code_prefix+call.request.email);
        //const_module是引入的const库（const.js），prefix前缀（是一个_code）+ 邮箱地址 = 表示一个key
        //举例：key[code_abc.163.com]-->uuid[e45u]
        console.log("query_res is", query_res)
        let uuqueId = query_res;
        if(query_res == null){
            //如果为空说明之前没有生成过验证码
            uniqueId = uuidv4();//uuidv4库生成验证码
            if(uniqueId.length > 4){
                uniqueId = uniqueId.substring(0,4);
            }
            //将key-value、验证码过期时间600秒设置进redis中
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix+call.request.email, uniqueId,600)
            //判断设置是否成功
            if(!bres){
                callback(null, { email: call.request.email,
                    error:const_module.Errors.RedisErr
                }); //调用失败的逻辑
                return;
            }
        }
        console.log("uniqueId is ", uniqueId)
        //写一段话发到别人的邮箱中
        let text_str =  '您的验证码为'+ uniqueId +'请三分钟内完成注册'
        //发送邮件的格式
        let mailOptions = {
            from: '17201763395@163.com',
            to: call.request.email,//请求携带的emil，往这个emil里面发
            subject: '验证码',//主题
            text: text_str,//文本（前面拼接好了格式）
        };
        //调用发送邮件
        let send_res = await emailModule.SendMail(mailOptions);//使用await等待发送完成才调用promise
        console.log("send res is ", send_res)//发送的结果

        callback(null, { email:  call.request.email,
            error:const_module.Errors.Success
        }); 
        
 
    }catch(error){//邮箱发送失败，看一下具体错误信息
        console.log("catch error is ", error)

        callback(null, { email:  call.request.email,
            error:const_module.Errors.Exception
        }); 
    }
    //这些调用返回会返回给GateServer
     
}

//主函数
function main() {
    //启动grpcServer，监听客户端连接信息
    var server = new grpc.Server()
    //message_proto的服务注册进去（目前只有获取验证码服务）
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })//两个参数：调用的服务名：接口
    //绑定，绑定好后做的回调：开启服务，打印
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        server.start()
        console.log('grpc server started')        
    })
}

main()