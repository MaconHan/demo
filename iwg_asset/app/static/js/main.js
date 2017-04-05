$(function () {
	showModal();
	select_log();
   	$(".create>a").click(function() {
   		$(".dropdown_menu").hide();
   		$(".nav").find(".sub_menu").hide();
   		$(".create>.dropdown_menu").show();
   		$(".create>.dropdown_menu>.sub_menu").show();
   		$(".data_div").remove();
   	})
   	
   	$(".select>a").click(function() {
   		$(".dropdown_menu").hide();
   		$("body").find(".sub_menu").hide();
   		$(".select>.dropdown_menu").show();
   		$(".select .dropdown_menu>.sub_menu").show();
   		$(".data_div").remove();
   	})
   	$(".nav .menu").click(function(){
        $("body").find(".menu").removeClass("menu_click");
        $(this).addClass("menu_click");
    })
    
    $(".user_info_list>a").click(function() {
   		$("body").find(".menu").removeClass("menu_click");
    	$("body").find(".sub_menu").hide();
   		$(".user_info_list>.dropdown_menu").show();
   		$(".user_info_list>.dropdown_menu>.sub_menu").show();
   		$(".data_div").remove();
    })
    $("#show_modify_modal").click(function() {
    	$(".dropdown_menu").hide();
    	$("#newPsd1").val("");
    	$("#newPsd2").val("");
    	$(".modify_password").css("display", "block");
    	$(".sub_menu").hide();
    	$("#cover").css("display","block");
    })
    $(".manage>a").click(function() {
    	$(".data_div").remove();
    	$("div.dropdown_menu").hide();
    })

    $("#cover").click(function() {
    	$("#gweui_info").remove();
    	$(".dropdown_menu").hide();
    	$(this).hide();
    	$(".modify_password").hide();
    	$(".add_user").hide();
    	$("body").css("overflow", "auto");
    })

    // 修改密码
    $("#modifyPsd").click(function() {
    	var newPsd1 = $("#newPsd1").val();
    	var newPsd2 = $("#newPsd2").val();
    	if (newPsd1 != newPsd2) {
    		prompt('两次密码不一致');
    		return;
    	}
    	$.ajax({
    		url: "/user/change/password/" + newPsd1,
    		type: 'put',
    		success: function(data, textStatus) {
    			if (data.err_code == 0) {
    				$("#cover").hide();
    				$(".modify_password").hide();
    				prompt("密码修改成功");
    			}else{
    				prompt(data.err_msg);
    			}
    		},
    		error: function(XMLHttpRequest, textStatus, errorThrown) {

    		}
    	})
    })
})
    

	function prompt(msg) {
		$("#note").text(msg);
        
        function out(){
            $("#note").animate({top: '-100px'}, 500, function(){ 
                $("#note").css({display:'none', top:'-100px'}); 
            });
        }

		if(!$("#note").is(':visible')){ 
			$("#note").css({display:'block', top:'-100px'}).animate({top: '10px'}, 500, function(){ 
				setTimeout(out, 2500);
			})
		}
	}
	//管理
   	function manage() {
   		$(".data_div").remove();
    	var tr = "";
		$.ajax({
			url: '/user/list',
			type: 'get',
			success: function(data, textStatus) {
				console.log(data);
				var obj = data.data;
				if (data.err_code == 0) {
					for (var i = 0; i < obj.length; i++) {
						tr += "<tr>" + 
							  	"<td>" + obj[i].id + "</td>" +
                                "<td>" + obj[i].name + "</td>" +
							  	"<td>" + obj[i].description + "</td>" +
							  	"<td>" + obj[i].permission_list + "</td>" +
								"<td>" + 
									"<button id='user_delete' onclick='user_delete(this);'>删除</button>" + 
									"<button id='reset_password' onclick='reset_password(this)'>密码复位</button>" +
								"</td>" +	
							  "</tr>";
					}
					var html = "<div class='data_div'>" +
									"<table class='table' id='manage_table'>" +
										"<caption>" + 
											"<button class='user_add_btn' id='user_add' onclick='show_add_user();'>增加用户</button>" + 
									    "</caption>" +
									   	"<thead>" +
						    		   		"<tr>" +
						    		   			"<th>ID</th>" +
						    		   			"<th>用户名</th>" +
                                                "<th>描述</th>" +
						    		   			"<th>权限列表</th>" +
						    		   			"<th>操作</th>" +
						    		   		"</tr>" + 
		    							"</thead>" +
		    							"<tbody>" + tr + "</tbody>" + 
		    						"</table>" +
		    					"</div>";
					$(".content").after(html);
				}else {
					prompt(data.err_msg);
				}
			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
                prompt(errorThrown);
			}

		})
    } 

    function clearInput() {
		$("#oui").html("");
		$("#rate_range").html("")
		$("#power_type").html("");
		$("#data_back").html("");
		$("#factory").html("");
		$("#size").val("20");
	}
	function showModal() {
		clearInput();
		$.ajax({
			url: "/gweui/define",
			type: "get",
			success: function(data, textStatus) {
				var obj = data.data;
				var oui = obj.oui;
				var rate_range = obj.rate_range;
				var power_type = obj.power_type;
				var data_back = obj.data_back;
				var factory = obj.factory;
				var oui_html = "";
				var rate_range_html = "";
				var power_type_html = "";
				var data_back_html = "";
				var factory_html = "";
				for(var i = 0; i < oui.length; i++) {
					oui_html += '<option value="' + oui[i].value + '">' + oui[i].description + '</option>';
				}
				for(var i = 0; i < rate_range.length; i++) {
					rate_range_html += '<option value="' + rate_range[i].value + '">' + rate_range[i].description + '</option>';
				}
				for(var i = 0; i < power_type.length; i++) {
					power_type_html += '<option value="' + power_type[i].value + '">' + power_type[i].description + '</option>';
				}
				for(var i = 0; i < data_back.length; i++) {
					data_back_html += '<option value="' + data_back[i].value + '">' + data_back[i].description +'</option>';
				}
				for(var i = 0; i < factory.length; i++) {
					factory_html += '<option value="' + factory[i].value + '">' + factory[i].description + '</option>';
				} 
				$("#oui").append(oui_html);
				$("#rate_range").append(rate_range_html);
				$("#power_type").append(power_type_html);
				$("#data_back").append(data_back_html);
				$("#factory").append(factory_html);
			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
				prompt("请求字段失败...")
			}
		});
	}

	function create() {
		$(".dropdown_menu").slideUp();
		$(".table").remove();
		var html = "";
		var oui = $("#oui").val();
		var rate_range = $("#rate_range").val();
		var power_type = $("#power_type").val();
		var data_back = $("#data_back").val();
		var factory = $("#factory").val();
		var size = $("#size").val();
		var create_req = {
			oui: oui,
			rate_range: rate_range,
			power_type: power_type,
			data_back: data_back,
			factory: factory,
			size: size
		}
		$.ajax({
			url: "/gweui/create",
			type: "post",
			data: JSON.stringify(create_req),
			contentType: "application/json",
			success: function(data, textStatus) {
				var obj = data.data;
                
                if (data.err_code > 0) {
                    prompt(data.err_msg);
                    return
                }
                
				var tbody = "";
				for(var i = 0; i < obj.length; i++) {
					tbody += "<tr>" +
								"<td>" + (i+1) + "</td>" +
								"<td>" + obj[i][0] + "</td>" +
								"<td>" + obj[i][1] + "</td>" +
							 "</tr>";
				}
				html = "<div class='data_div'>" +
							"<ul><li><div class='export_btn' onclick='export_encrypt()'>加密导出</div></li><li><div onclick='export_txt()' class='export_btn'>原文导出</div></li></ul>" +
							"<table class='table'>" +
								"<thead>" + 
									"<tr>" +
										"<th>序号</th>" + 
										"<th>GWEUI</th>" +
										"<th>密钥</th>" +
									"</tr>" +
								"</thead>" +
								"<tbody>" + tbody + "</tbody>" +
						   "</table>" +
					   "</div>";
				$(".content").after(html);
			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
				
			}
		});
	}
	function export_encrypt() {
		$.ajax({
			url: "/gweui/export",
			type: "get",
			success:function(data, textStatus) {
				if (data.err_code == 0) {
					var fileName = data.data.filename;
					window.location.href="/download/" + fileName;
				}else{
					prompt(data.err_msg);
				}
			},
			error: function(XMLHttpRequest, textStatus,errorThrown) {
				// prompt(errorThrown);
			}
		});
	}
	function export_txt() {
		$.ajax({
			url: "/gweui/export/txt",
			type: "get",
			success:function(data, textStatus) {
				if (data.err_code == 0) {
					var fileName = data.data.filename;
					window.location.href="/download/" + fileName;
				}else{
					prompt(data.err_msg);
				}
			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
				// prompt(errorThrown);
			}
		});
	}

	// 查询gweui
	function query() {
		var html = "<div class='data_div query_gweui_div'>" +
						"<ul>" +
							"<li>" + 
								"<div class='export_btn' onclick='export_log_info($(\"#query_list tbody\"))'>导出</div>" +
							"</li>" + 
						"</ul>" +
						"<table id='query_list' class='table'>" + 
							"<thead>" + 
								"<tr>" + 
									"<th>GWEUI</th>" +
									"<th>密钥</th>" +
                                    "<th>创建时间</th>" +
									"<th>状态</th>" +
								"</tr>" +
							"</thead>" +
							"<tbody></tbody>" +
						"</table>" +
				   "</div>";
		var tbody = "";
		var gweui = $("#gweui").val();
		if (gweui == null || gweui == "") {
			prompt('请输入GWEUI！');
			return;
		}
		if (!($(".data_div").length && $(".data_div").length > 0)) {
			$(".content").after(html);
		}
		$("#gweui").val("");
		$.ajax({
			url: "/gweui/get/" + gweui,
			type: "get",
			data: gweui,
			success: function(data, textStatus) {
				var obj = data.data;
				
				if (data.err_code == 0) {
					var status = obj.status;
					if (obj.status == -1) {
						status = "弃用";
					}else if(obj.status == 0) {
						status = "未分配";
					}else if(obj.status == 1) {
						status = "已分配";
					}
					tbody = "<tr>" + 
								"<td>" + obj.gweui + "</td>" +
								"<td>" + obj.secret_key + "</td>" +
                                "<td>" + obj.create_time + "</td>" +
								"<td>" + status + "</td>" +
							 "</tr>";
					if ($(".data_div").length && $(".data_div").length > 0) {
						$(".data_div table").append(tbody);
					}
				}else{
					prompt(data.err_msg);
				}

			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
			} 
		});
	}

	//查询日志
	function select_log() {
		$(".data_div").remove();
		$("body").find(".dropdown_menu").hide();
		$.ajax({
			url: "/logs/get/0/50",
			type: "get",
			success: function(data, textStatus) {
				var obj = data.data;
				var tbody = "";
				var html = "";
				for(var i = 0; i < obj.length; i++) {
					var obj = data.data;
					
					tbody += "<tr onclick='query_gweui_info(this);'>" + 
							 	"<td>" + obj[i].id + "</td>" +
							 	"<td>" + obj[i].serial + "</td>" +
							 	"<td>" + obj[i].operate_time + "</td>" +
							 	"<td>" + obj[i].operate_type + "</td>" +
							 	"<td>" + obj[i].operator + "</td>" +
							 	"<td>" + obj[i].message + "</td>" +
							"</tr>";
				}		
				html =	"<div class='data_div'><table class='table cover' id='log_info'>" + 
						 	"<thead>" + 
						 		"<tr>" + 
						 			"<th>ID</th>" +
						 			"<th>序列号</th>" +
						 			"<th>操作时间</th>" +
						 			"<th>操作类型</th>" +
						 			"<th>操作人</th>" +
						 			"<th>信息</th>" +
						 		"</tr>" +
						 	"</thead>" + 
						 	"<tbody>" + tbody + "</tbody>" +
						"</table></div>";
				$(".content").after(html);
			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
			}
		})
	}
	// 日志详情  
	// 拼接html
	function joint_html(id, string) {
		var html = "<div id='gweui_info'>" +
						"<div class='preview able' id='preview_info' onclick='preview_info(this)'>" +
							"<span></span>" +
						"</div>" +
						"<div class='next able' id='next_info' onclick='next_info(this)'>" +
							"<span></span>" +
						"</div>" +
						"<div id='export_log_info' onclick='export_log_info( $(\"#log_table tbody\"));'>导出</div>" +
						"<div class='table_scroll'>" +		
							"<table class='table' id='log_table' log_id='" + id + "'>" + 
								"<caption>日志ID为" + id + "的详细信息</caption>" +			
								"<thead>" + 
									"<tr>" + 
										"<th>GWEUI</th>" +
										"<th>密钥</th>" +
										"<th>状态</th>" +
									"</tr>" +
								"</thead>" +
								"<tbody>" + string + "</tbody>" +
							"</table>" +
						"</div>" + 
						
					"</div>";
		$(".content").after(html);
	}
	function query_gweui_info(ele) {
		var tr = $("#log_info tbody").find("tr");
		var id = $(ele).find("td:eq(0)").text();
		$("#cover").css('display','block');
		$("body").css("overflow","hidden");
		var html = 
		$.ajax({
			url: "gweui/get/log_id/" + id,
			type: "get",
			success: function(data, textStatus) {
				var obj = data.data;
				var tbody = "";
				var html = "";
				for(var i = 0; i < obj.length; i++) {
					var status = obj[i].status;
					if (status == -1) {
						status = "弃用";
					}else if(status == 0) {
						status = "未分配";
					}else if(status == 1) {
						status = "已分配";
					}
					tbody += "<tr>" + 
								"<td>" + obj[i].gweui + "</td>" +
								"<td>" + obj[i].secret_key + "</td>" +
								"<td>" + status + "</td>" +
							 "</tr>";
				}
				joint_html(id, tbody);
			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
			}
		});
	}
	// 上一条日志详情
	function preview_info(ele) {
		var log_id = $(ele).parent("div").find("table").attr("log_id"); 
		var tr = $("#log_info tbody").find("tr");
		for(var i = 0; i < tr.length; i++) {
			if($(tr[0]).find("td:eq(0)").text() == log_id) {
				$("#preview_info span").attr("disabled","disabled").addClass("disabled");
				return;
			}
			if ($(tr[i]).find("td:eq(0)").text() == log_id) {
				$(ele).parent("div").find('table').remove();
				var id = $(tr[i-1]).find("td:eq(0)").text();
				$.ajax({
					url: "gweui/get/log_id/" + id,
					type: "get",
					success: function(data, textStatus) {
						var obj = data.data;
						var tbody = "";
						var html = "";
						for(var i = 0; i < obj.length; i++) {
                            var status = obj[i].status;
                            if (status == -1) {
                                status = "弃用";
                            }else if(status == 0) {
                                status = "未分配";
                            }else if(status == 1) {
                                status = "已分配";
                            }
							tbody += "<tr>" + 
										"<td>" + obj[i].gweui + "</td>" +
										"<td>" + obj[i].secret_key + "</td>" +
										"<td>" + status + "</td>" +
									"</tr>";
						}
						html = 	"<table class='table' id='log_table' log_id='" + id + "'>" + 
									"<caption>日志ID为" + id + "的详细信息</caption>" +			
									"<thead>" + 
										"<tr>" + 
											"<th>GWEUI</th>" +
											"<th>密钥</th>" +
											"<th>状态</th>" +
										"</tr>" +
									"</thead>" +
									"<tbody>" + tbody + "</tbody>" +
								"</table>";
						$(".table_scroll").append(html);
					},
					error: function(XMLHttpRequest, textStatus, errorThrown) {
					}
				});
			}
		}
		$("#next_info span").removeClass("disabled").attr("disabled","false");
	}
	// 下一条信息
	function next_info(ele) {
		var log_id = $(ele).parent("div").find("table").attr("log_id"); 
		var tr = $("#log_info tbody").find("tr");
		for(var i = 0; i < tr.length; i++) {
			if($(tr[tr.length-1]).find("td:eq(0)").text() == log_id) {
				$("#next_info span").attr("disabled","disabled").addClass("disabled");
				return;
			}
			if ($(tr[i]).find("td:eq(0)").text() == log_id) {
				$(ele).parent("div").find('table').remove();
				var id = $(tr[i+1]).find("td:eq(0)").text();
				$.ajax({
					url: "gweui/get/log_id/" + id,
					type: "get",
					success: function(data, textStatus) {
						var obj = data.data;
						var tbody = "";
						var html = "";
						for(var i = 0; i < obj.length; i++) {
                            var status = obj[i].status;
                            if (status == -1) {
                                status = "弃用";
                            }else if(status == 0) {
                                status = "未分配";
                            }else if(status == 1) {
                                status = "已分配";
                            }
                            
							tbody += "<tr>" + 
										"<td>" + obj[i].gweui + "</td>" +
										"<td>" + obj[i].secret_key + "</td>" +
										"<td>" + status + "</td>" +
									"</tr>";
						}
						html = "<table class='table' id='log_table' log_id='" + id + "'>" + 
									"<caption>日志ID为" + id + "的详细信息</caption>" +
									"<thead>" + 
										"<tr>" + 
											"<th>GWEUI</th>" +
											"<th>密钥</th>" +
											"<th>状态</th>" +
										"</tr>" +
									"</thead>" +
									"<tbody>" + tbody + "</tbody>" +
								"</table>";
						$(".table_scroll").append(html);
					},
					error: function(XMLHttpRequest, textStatus, errorThrown) {
					}
				});
			}
		}
		$("#preview_info span").removeClass("disabled").attr("disabled","false");
	}
	function export_log_info(ele) {
		var gweuiArr = [];
		var tr = ele.find("tr");
		for(var i = 0; i < tr.length; i++) {
			gweuiArr.push($(tr[i]).find("td:eq(0)").text());
		}
		$.ajax({
			url: "/gweui/export",
			type: "post",
			data: JSON.stringify(gweuiArr),
			contentType: "application/json",
			success:function(data, textStatus) {
				if (data.err_code == 0) {
					var fileName = data.data.filename;
					window.location.href="/download/" + fileName;
				} else {
					prompt(data.err_msg);
				}
			},
			error: function(XMLHttpRequest, textStatus,errorThrown) {
				prompt(errorThrown);
			}
		});
	}
	// 管理
	// 删除用户
	function user_delete(ele) {
		var r=confirm("确定删除此用户吗?");
		if (r==false){
  			return;
  		}
		var user_name = $(ele).parent().parent("tr").find("td:eq(2)").text();
		$.ajax({
			url: "/user/delete/" + user_name,
			type: "delete",
			success:function(data, textStatus) {
				if (data.err_code == 0) {
					manage();
					prompt("操作成功")
				} else {
					prompt(data.err_msg);
				}
			},
			error: function(XMLHttpRequest, textStatus,errorThrown) {
				prompt(errorThrown);
			}
		});
	}
	//密码复位
	function reset_password(ele) {
		var r=confirm("确定复位此用户的密码吗?");
		if (r==false){
  			return;
  		}
		var user_name = $(ele).parent().parent("tr").find("td:eq(2)").text();
		var data = {
	    	"name": user_name,
	    	"password": ""
		};
		$.ajax({
			url: "/user/change/password",
			type: "put",
			data: JSON.stringify(data),
			contentType: "application/json",
			success:function(data, textStatus) {
				if (data.err_code == 0) {
					prompt("操作成功")
				} else {
					prompt(data.err_msg);
				}
			},
			error: function(XMLHttpRequest, textStatus,errorThrown) {
				prompt(errorThrown);
			}
		});
	}
	// 增加用户
	function show_add_user() {
		$("#add_username").val("");
		$("#add_password1").val("");
		$("#add_password2").val("");
    	$(".add_user").css("display", "block");
    	$(".sub_menu").hide();
    	$("#cover").css("display","block");
	}

	function add_user() {
		if ($("#add_password1").val() != $("#add_password2").val()) {
			prompt('两次密码不一致');
    		return;
		}

		var data = {
	    	"name": $("#add_username").val(),
	    	"password": $("#add_password1").val(),
	    	"permission_list": "all"
		};
		$.ajax({
			url: "/user/add",
			type: "post",
			data: JSON.stringify(data),
			contentType: "application/json",
			success:function(data, textStatus) {
				if (data.err_code == 0) {
					manage();
					prompt("操作成功");
				} else {
					prompt(data.err_msg);
				}
			},
			error: function(XMLHttpRequest, textStatus,errorThrown) {
				prompt(errorThrown);
			}
		});
		$(".add_user").hide();
		$("#cover").hide();
	}

