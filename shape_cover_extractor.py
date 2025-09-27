#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import requests
import cv2
import numpy as np
import os
import re
from urllib.parse import urlparse
from bs4 import BeautifulSoup
import time

class ShapeBasedCoverExtractor:
    def __init__(self):
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
        })
    
    def extract_game_name_from_url(self, url):
        """从URL中提取游戏名称"""
        try:
            path = urlparse(url).path
            game_name = path.replace('/trainer/', '').replace('-trainer/', '').replace('-trainer', '')
            game_name = game_name.replace('-', ' ').title()
            return game_name
        except:
            return "Unknown_Game"
    
    def get_trainer_image_from_page(self, url):
        """从页面中提取修改器截图URL"""
        try:
            print(f"正在访问页面: {url}")
            response = self.session.get(url)
            response.raise_for_status()
            
            soup = BeautifulSoup(response.content, 'html.parser')
            img_tags = soup.find_all('img')
            
            # 优先查找包含trainer、cheat等关键词的图片
            for img in img_tags:
                src = img.get('src', '')
                alt = img.get('alt', '').lower()
                
                if any(keyword in alt for keyword in ['trainer', 'cheat', 'hack']) and any(ext in src for ext in ['.png', '.jpg', '.jpeg']):
                    if src.startswith('//'):
                        src = 'https:' + src
                    elif src.startswith('/'):
                        base_url = f"{urlparse(url).scheme}://{urlparse(url).netloc}"
                        src = base_url + src
                    return src
            
            # 如果没找到，查找大尺寸图片
            for img in img_tags:
                src = img.get('src', '')
                if any(ext in src for ext in ['.png', '.jpg', '.jpeg']):
                    width = img.get('width')
                    height = img.get('height')
                    
                    if width and height:
                        try:
                            if int(width) > 800 and int(height) > 600:
                                if src.startswith('//'):
                                    src = 'https:' + src
                                elif src.startswith('/'):
                                    base_url = f"{urlparse(url).scheme}://{urlparse(url).netloc}"
                                    src = base_url + src
                                return src
                        except:
                            continue
            
            print(f"未找到合适的修改器截图")
            return None
            
        except Exception as e:
            print(f"获取页面失败: {e}")
            return None
    
    def extract_cover_by_shape_analysis(self, image):
        """基于形状和边缘分析提取封面（最佳方法）"""
        try:
            height, width = image.shape[:2]
            print(f"开始形状分析，图片尺寸: {width} x {height}")
            
            # 专注于左上角区域（游戏封面通常在此位置）
            roi_width = int(width * 0.4)
            roi_height = int(height * 0.8)
            roi = image[:roi_height, :roi_width]
            
            print(f"分析区域尺寸: {roi_width} x {roi_height}")
            
            # 转换为灰度
            gray = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
            
            # 高斯模糊以减少噪声
            blurred = cv2.GaussianBlur(gray, (3, 3), 0)
            
            # 多层边缘检测以获得更好的边缘信息
            edges1 = cv2.Canny(blurred, 30, 80)
            edges2 = cv2.Canny(blurred, 50, 120)
            edges3 = cv2.Canny(blurred, 70, 150)
            
            # 合并多层边缘结果
            edges = cv2.bitwise_or(edges1, edges2)
            edges = cv2.bitwise_or(edges, edges3)
            
            # 形态学操作以连接断裂的边缘
            kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
            edges = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)
            edges = cv2.morphologyEx(edges, cv2.MORPH_DILATE, kernel)
            
            # 查找轮廓
            contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            print(f"找到 {len(contours)} 个轮廓")
            
            # 筛选可能的封面矩形区域
            candidates = []
            filtered_reasons = []  # 记录过滤原因
            
            for i, contour in enumerate(contours):
                area = cv2.contourArea(contour)
                
                # 设置最小面积阈值（根据图片大小动态调整）
                min_area = max(5000, (roi_width * roi_height) * 0.015)  # 降低面积要求
                
                if area > min_area:
                    # 近似为多边形
                    epsilon = 0.02 * cv2.arcLength(contour, True)
                    approx = cv2.approxPolyDP(contour, epsilon, True)
                    
                    # 至少是四边形
                    if len(approx) >= 4:
                        x, y, w, h = cv2.boundingRect(contour)
                        aspect_ratio = float(w) / h
                        
                        # 记录详细的过滤信息
                        reason = []
                        passed = True
                        
                        # 放宽长宽比要求
                        if not (0.5 < aspect_ratio < 1.2):  # 放宽从1.0到1.2
                            reason.append(f"长宽比{aspect_ratio:.2f}不符合要求(0.5-1.2)")
                            passed = False
                        
                        # 放宽最小尺寸要求
                        if w < 80 or h < 120:  # 降低最小尺寸要求
                            reason.append(f"尺寸太小({w}x{h}，要求>80x120)")
                            passed = False
                        
                        # 放宽边缘距离要求
                        if x < 3 or y < 3:  # 降低边缘距离要求
                            reason.append(f"太靠近边缘(x={x}, y={y})")
                            passed = False
                        
                        if passed:
                            # 计算质量得分
                            cover_region = roi[y:y+h, x:x+w]
                            quality_score = self.calculate_region_quality(cover_region)
                            
                            candidates.append((x, y, w, h, area, quality_score, i))
                            print(f"✓ 候选区域 {i}: 位置({x},{y}) 尺寸({w}x{h}) 比例{aspect_ratio:.2f} 面积{area:.0f} 质量{quality_score:.2f}")
                        else:
                            filtered_reasons.append(f"区域 {i}: {'; '.join(reason)}")
                    else:
                        filtered_reasons.append(f"区域 {i}: 多边形顶点数{len(approx)}<4")
                else:
                    filtered_reasons.append(f"区域 {i}: 面积{area:.0f}<{min_area:.0f}")
            
            # 显示过滤信息
            if len(filtered_reasons) > 0:
                print(f"过滤掉的区域: {len(filtered_reasons)}个")
                for reason in filtered_reasons[:5]:  # 只显示前5个原因
                    print(f"  - {reason}")
                if len(filtered_reasons) > 5:
                    print(f"  ... 还有{len(filtered_reasons)-5}个被过滤")
            
            if candidates:
                # 按综合得分排序（面积 × 质量得分）
                candidates.sort(key=lambda x: x[4] * x[5], reverse=True)
                best_candidate = candidates[0]
                x, y, w, h, area, quality, idx = best_candidate
                
                print(f"选择最佳候选区域 {idx}: 位置({x},{y}) 尺寸({w}x{h}) 综合得分{area * quality:.0f}")
                
                # 提取封面区域
                cover = roi[y:y+h, x:x+w]
                
                # 进一步优化边框
                optimized_cover = self.remove_cover_borders(cover)
                
                return optimized_cover if optimized_cover is not None else cover
            else:
                print("❌ 未找到符合条件的封面区域")
                print(f"总轮廓数: {len(contours)}, 通过面积筛选: {len([c for c in contours if cv2.contourArea(c) > max(5000, (roi_width * roi_height) * 0.015)])}")
                
                # 尝试降级处理：选择最大的几个轮廓作为备选
                print("尝试降级处理策略1: 降低要求...")
                backup_candidates = []
                
                # 策略1: 大幅降低面积要求，放宽长宽比
                for i, contour in enumerate(contours):
                    area = cv2.contourArea(contour)
                    if area > max(2000, (roi_width * roi_height) * 0.005):  # 进一步降低面积要求
                        x, y, w, h = cv2.boundingRect(contour)
                        aspect_ratio = float(w) / h
                        
                        # 更宽松的条件
                        if (0.2 < aspect_ratio < 2.5 and w > 40 and h > 60):
                            cover_region = roi[y:y+h, x:x+w]
                            quality_score = self.calculate_region_quality(cover_region)
                            backup_candidates.append((x, y, w, h, area, quality_score, i))
                            print(f"  策略1候选 {i}: 位置({x},{y}) 尺寸({w}x{h}) 比例{aspect_ratio:.2f} 质量{quality_score:.2f}")
                
                if backup_candidates:
                    backup_candidates.sort(key=lambda x: x[4] * x[5], reverse=True)
                    best_backup = backup_candidates[0]
                    x, y, w, h, area, quality, idx = best_backup
                    print(f"✓ 使用策略1区域 {idx}: 位置({x},{y}) 尺寸({w}x{h})")
                    
                    cover = roi[y:y+h, x:x+w]
                    optimized_cover = self.remove_cover_borders(cover)
                    return optimized_cover if optimized_cover is not None else cover
                
                # 策略2: 如果策略1失败，使用固定位置提取
                print("尝试降级处理策略2: 固定位置提取...")
                fallback_cover = self.extract_fallback_by_position(roi)
                if fallback_cover is not None:
                    print("✓ 使用固定位置提取成功")
                    return fallback_cover
                
                # 策略3: 最后的备用方案 - 选择最大轮廓
                print("尝试降级处理策略3: 选择最大轮廓...")
                if contours:
                    largest_contour = max(contours, key=cv2.contourArea)
                    largest_area = cv2.contourArea(largest_contour)
                    
                    if largest_area > 1000:  # 最低面积要求
                        x, y, w, h = cv2.boundingRect(largest_contour)
                        if w > 30 and h > 50:  # 最基本的尺寸要求
                            print(f"✓ 使用最大轮廓: 位置({x},{y}) 尺寸({w}x{h}) 面积{largest_area:.0f}")
                            cover = roi[y:y+h, x:x+w]
                            optimized_cover = self.remove_cover_borders(cover)
                            return optimized_cover if optimized_cover is not None else cover
                
                print("❌ 所有降级策略都失败了")
                
                return None
                
        except Exception as e:
            print(f"形状分析失败: {e}")
            import traceback
            traceback.print_exc()
            
            # 作为最后的备用方案，使用固定位置提取
            print("尝试最终备用方案：固定位置提取...")
            fallback_cover = self.extract_fallback_by_position(roi if 'roi' in locals() else image[:int(image.shape[0]*0.8), :int(image.shape[1]*0.4)])
            return fallback_cover
    
    def extract_fallback_by_position(self, roi):
        """备用方案：固定位置提取"""
        try:
            height, width = roi.shape[:2]
            
            # 尝试几个典型的封面位置
            positions = [
                # (x_ratio, y_ratio, w_ratio, h_ratio)
                (0.02, 0.1, 0.35, 0.6),   # 左上角，较大区域
                (0.05, 0.15, 0.3, 0.5),   # 左上角，中等区域
                (0.01, 0.05, 0.4, 0.7),   # 左侧，大区域
                (0.03, 0.2, 0.25, 0.4),   # 左上角，小区域
            ]
            
            best_cover = None
            best_score = 0
            
            for i, (x_ratio, y_ratio, w_ratio, h_ratio) in enumerate(positions):
                x = int(width * x_ratio)
                y = int(height * y_ratio)
                w = int(width * w_ratio)
                h = int(height * h_ratio)
                
                # 确保不超出边界
                x = max(0, min(x, width - 1))
                y = max(0, min(y, height - 1))
                w = min(w, width - x)
                h = min(h, height - y)
                
                if w > 50 and h > 80:  # 基本尺寸要求
                    cover = roi[y:y+h, x:x+w]
                    score = self.calculate_region_quality(cover)
                    
                    print(f"  固定位置 {i+1}: 位置({x},{y}) 尺寸({w}x{h}) 质量{score:.2f}")
                    
                    if score > best_score:
                        best_score = score
                        best_cover = cover
            
            return best_cover if best_score > 0.5 else None
            
        except Exception as e:
            print(f"固定位置提取失败: {e}")
            return None
    
    def calculate_region_quality(self, region):
        """计算区域质量得分（优化版本）"""
        try:
            if region is None or region.size == 0:
                return 0
            
            height, width = region.shape[:2]
            
            # 尺寸加分（更大的区域通常更好）
            size_score = min(2.0, (width * height) / 50000)  # 归一化尺寸得分
            
            # 转换为灰度
            gray = cv2.cvtColor(region, cv2.COLOR_BGR2GRAY)
            
            # 计算纹理复杂度（标准差）
            texture_std = np.std(gray)
            texture_score = min(2.0, texture_std / 30)  # 归一化纹理得分
            
            # 计算边缘密度（更宽松的参数）
            edges = cv2.Canny(gray, 30, 100)  # 降低边缘检测阈值
            edge_density = np.count_nonzero(edges) / (height * width)
            edge_score = min(2.0, edge_density * 15)  # 归一化边缘得分
            
            # 计算颜色分布多样性
            hist = cv2.calcHist([gray], [0], None, [128], [0, 256])  # 使用更少的bin
            color_diversity = np.count_nonzero(hist)
            diversity_score = min(2.0, color_diversity / 30)  # 归一化多样性得分
            
            # 计算亮度分布（避免过暗或过亮）
            mean_brightness = np.mean(gray)
            if 30 < mean_brightness < 220:  # 更宽松的亮度范围
                brightness_score = 1.0
            else:
                brightness_score = 0.3
            
            # 计算对比度
            contrast = gray.max() - gray.min()
            contrast_score = min(1.0, contrast / 100)
            
            # 综合得分（权重优化）
            quality_score = (
                size_score * 0.2 +        # 尺寸权重
                texture_score * 0.25 +    # 纹理权重
                edge_score * 0.2 +        # 边缘权重
                diversity_score * 0.15 +  # 多样性权重
                brightness_score * 0.1 +  # 亮度权重
                contrast_score * 0.1      # 对比度权重
            )
            
            return max(0.1, min(quality_score, 8.0))  # 确保有最小得分，避免0得分
            
        except Exception as e:
            print(f"质量评分计算错误: {e}")
            return 1.0  # 返回默认得分而不是0
    
    def remove_cover_borders(self, cover_image):
        """智能去除封面图片的多余边框"""
        if cover_image is None:
            return None
        
        try:
            # 转换为灰度
            gray = cv2.cvtColor(cover_image, cv2.COLOR_BGR2GRAY)
            
            # 使用更宽松的边缘检测
            edges = cv2.Canny(gray, 30, 100)
            
            # 形态学操作连接边缘
            kernel = np.ones((3,3), np.uint8)
            edges = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)
            
            # 查找轮廓
            contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            if contours:
                # 找到最大的内部轮廓
                largest_contour = max(contours, key=cv2.contourArea)
                x, y, w, h = cv2.boundingRect(largest_contour)
                
                # 检查裁切区域是否合理（不会过度裁切）
                original_area = cover_image.shape[0] * cover_image.shape[1]
                crop_area = w * h
                
                if (crop_area > original_area * 0.5 and  # 裁切后面积不少于原来的50%
                    w > cover_image.shape[1] * 0.6 and   # 宽度不少于原来的60%
                    h > cover_image.shape[0] * 0.6):     # 高度不少于原来的60%
                    
                    # 添加小的边距以避免裁切过紧
                    margin = 3
                    x = max(0, x - margin)
                    y = max(0, y - margin)
                    w = min(cover_image.shape[1] - x, w + 2 * margin)
                    h = min(cover_image.shape[0] - y, h + 2 * margin)
                    
                    return cover_image[y:y+h, x:x+w]
            
            # 如果边框检测失败，返回原图
            return cover_image
            
        except:
            return cover_image
    
    def process_single_game(self, url, base_output_dir="shape_extraction_results"):
        """处理单个游戏，只使用形状分析方法"""
        game_name = self.extract_game_name_from_url(url)
        print(f"\n处理游戏: {game_name}")
        print("=" * 60)
        
        # 创建游戏专属文件夹
        safe_name = re.sub(r'[<>:"/\\|?*]', '_', game_name.replace(' ', '_'))
        game_dir = os.path.join(base_output_dir, safe_name)
        os.makedirs(game_dir, exist_ok=True)
        
        # 获取修改器截图URL
        image_url = self.get_trainer_image_from_page(url)
        if not image_url:
            print(f"无法找到 {game_name} 的修改器截图")
            return False
        
        try:
            print(f"正在下载图片: {image_url}")
            response = self.session.get(image_url)
            response.raise_for_status()
            
            # 保存原图
            original_path = os.path.join(game_dir, "trainer_original.png")
            with open(original_path, 'wb') as f:
                f.write(response.content)
            print(f"原图已保存: {original_path}")
            
            # 使用OpenCV读取图片
            image = cv2.imread(original_path)
            if image is None:
                print(f"无法读取图片: {original_path}")
                return False
            
            # 使用形状分析方法提取封面
            cover = self.extract_cover_by_shape_analysis(image)
            
            if cover is not None:
                cover_path = os.path.join(game_dir, f"{safe_name}_cover.png")
                success = cv2.imwrite(cover_path, cover)
                
                if success:
                    print(f"✓ 封面提取成功: {cover_path}")
                    print(f"  封面尺寸: {cover.shape[1]} x {cover.shape[0]}")
                    return True
                else:
                    print(f"✗ 封面保存失败")
                    return False
            else:
                print(f"✗ 未能提取到封面")
                return False
                
        except Exception as e:
            print(f"处理失败: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def process_multiple_games(self, urls):
        """处理多个游戏"""
        print("基于形状分析的游戏封面提取器")
        print("=" * 60)
        print(f"将处理 {len(urls)} 个游戏...")
        
        results = {}
        
        for i, url in enumerate(urls, 1):
            print(f"\n[{i}/{len(urls)}] 处理URL: {url}")
            
            try:
                success = self.process_single_game(url)
                game_name = self.extract_game_name_from_url(url)
                results[game_name] = success
                
                # 添加延时，避免请求过快
                if i < len(urls):
                    time.sleep(1)
                    
            except Exception as e:
                print(f"处理失败: {e}")
                game_name = self.extract_game_name_from_url(url)
                results[game_name] = False
        
        # 输出处理结果摘要
        print("\n" + "=" * 60)
        print("处理结果摘要:")
        print("=" * 60)
        
        success_count = 0
        for game_name, success in results.items():
            status = "✓ 成功" if success else "✗ 失败"
            print(f"{game_name}: {status}")
            if success:
                success_count += 1
        
        print(f"\n总计: {success_count}/{len(urls)} 个游戏处理成功")
        print(f"结果保存在: shape_extraction_results/ 文件夹")
        
        return results


def main():
    # 要处理的游戏URL列表
    game_urls = [
        "https://flingtrainer.com/trainer/dying-light-the-beast-trainer/",
        "https://flingtrainer.com/trainer/sekiro-shadows-die-twice-trainer/",
        "https://flingtrainer.com/trainer/legend-of-mortal-trainer/",
        "https://flingtrainer.com/trainer/the-riftbreaker-trainer/"
    ]
    
    extractor = ShapeBasedCoverExtractor()
    results = extractor.process_multiple_games(game_urls)
    
    print("\n" + "=" * 60)
    print("处理完成！")
    print("每个游戏只生成一个最佳的封面文件：*_cover.png")
    print("基于形状分析的方法，效果最佳且稳定。")


if __name__ == "__main__":
    main()